#include "MCPServer.h"
#include "../OSCLogger.h"
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSFileManager.h"
#include "MCPGeneratedToolLoader.h"
#include "MCPParameterRegistry.h"
#include "tools/SessionTools.h"
#include "tools/InputTools.h"
#include "tools/OutputTools.h"
#include "tools/ReverbTools.h"
#include "tools/SnapshotTools.h"
#include "tools/UndoTools.h"
#include "tools/SetParameterTool.h"
#include "tools/SetParameterBatchTool.h"
#include "tools/DescribeParametersTool.h"
#include "tools/StateInspectionTools.h"
#include "tools/ChannelLifecycleTools.h"

namespace WFSNetwork
{

MCPServer::MCPServer (WFSValueTreeState& state,
                      WFSFileManager& fileMgr,
                      OSCLogger& networkLogger,
                      const juce::File& generatedToolsJson,
                      const juce::File& knowledgeResourcesDir)
    : valueTreeState (state),
      fileManager (fileMgr),
      generatedToolsJsonPath (generatedToolsJson)
{
    mcpLogger        = std::make_unique<MCPLogger> (networkLogger);
    registry         = std::make_unique<MCPToolRegistry>();
    changeRecords    = std::make_unique<MCPChangeRecordBuffer>();
    undoEngine       = std::make_unique<MCPUndoEngine> (state, *changeRecords);
    resourceRegistry = std::make_unique<MCPResourceRegistry> (knowledgeResourcesDir);
    promptRegistry   = std::make_unique<MCPPromptRegistry>();
    tierEnforcement  = std::make_unique<MCPTierEnforcement>();
    dispatcher       = std::make_unique<MCPDispatcher> (state, *registry, *changeRecords,
                                                        *undoEngine,
                                                        *resourceRegistry, *promptRegistry,
                                                        *tierEnforcement,
                                                        *mcpLogger);
    transport        = std::make_unique<MCPTransport> (*mcpLogger);

    mcpLogger->logInfo ("Loaded " + juce::String (resourceRegistry->size())
                        + " knowledge resources from " + knowledgeResourcesDir.getFullPathName());
    mcpLogger->logInfo ("Loaded " + juce::String (promptRegistry->size())
                        + " workflow prompts (inline catalog)");

    // Parameter registry — parses generated_tools.json once into the
    // singleton consumed by mcp_describe_parameters and by the
    // wfs_set_parameter whitelist. Must run before either tool is
    // registered, but can run before or after the loader pass.
    MCPParameterRegistry::getInstance().loadFromManifest (generatedToolsJson, *mcpLogger);

    // Phase 2 — register the auto-generated tool surface FIRST. The
    // hand-written tools registered below silently overwrite by name,
    // so when a Phase-1 hand-written tool collides with a generated one
    // (input.set_attenuation today), the hand-written version wins
    // (preserves its richer operator description).
    Tools::Generated::loadGeneratedTools (*registry, state, generatedToolsJson, *mcpLogger);

    // Phase 1 hand-written tools — registered after the generated set so
    // that any name collision keeps the hand-written variant.
    registry->registerTool (Tools::Session::describe (state));
    registry->registerTool (Tools::Input::describeSetName (state));
    registry->registerTool (Tools::Input::describeSetCartesian (state));
    registry->registerTool (Tools::Input::describeSetAttenuation (state));
    registry->registerTool (Tools::Output::describeSetCartesian (state));
    registry->registerTool (Tools::Reverb::describeSetCartesian (state));
    registry->registerTool (Tools::Snapshot::describe (fileMgr));

    // Phase 2 Block 3 — generic escape-hatch tool. Lets the AI hit a
    // parameter the auto-generated surface didn't cover. Trusted-caller
    // semantics: no clamping, exact variable names required.
    registry->registerTool (Tools::SetParameter::describe (state));

    // Batch primitive — atomic multi-write with one undo entry. Mirrors
    // wfs_set_parameter's per-entry shape; pre-validates everything,
    // then applies and records as a single ChangeRecord with subWrites.
    registry->registerTool (Tools::SetParameterBatch::describe (state));

    // Read-only registry tool — surfaces every known parameter so the AI
    // can plan writes from the schema instead of guessing. Must come after
    // MCPParameterRegistry::loadFromManifest above.
    registry->registerTool (Tools::DescribeParameters::describeTool());

    // Read-only deep-state tools — globals + per-channel full dump,
    // complementing session_get_state's per-channel summary.
    registry->registerTool (Tools::StateInspection::describeGlobalState (state));
    registry->registerTool (Tools::StateInspection::describeChannelFull (state));

    // Channel lifecycle — tier-2 wrappers that bump the global channel
    // counts by 1 (auto-gen `system_i_o_set_*_channels` is tier 3 because
    // it accepts arbitrary counts). Lets the AI script "create channel
    // then write to it" flows from a blank session.
    for (const auto& kind : { juce::String ("input"),
                               juce::String ("output"),
                               juce::String ("reverb") })
    {
        registry->registerTool (Tools::ChannelLifecycle::describeCreate (state, kind));
        registry->registerTool (Tools::ChannelLifecycle::describeDelete (state, kind));
    }

    // Undo / redo tools — Phase 5a wires the first two to the real engine.
    // mcp.get_ai_change_history remains a read-only query over the ring buffer.
    registry->registerTool (Tools::Undo::describeUndo (*undoEngine));
    registry->registerTool (Tools::Undo::describeRedo (*undoEngine));
    registry->registerTool (Tools::Undo::describeGetHistory (*changeRecords));

    // Wire the transport's POST /mcp callback to the dispatcher.
    transport->setRequestHandler ([disp = dispatcher.get()] (const juce::String& body,
                                                             const juce::String& clientIP,
                                                             int clientPort)
    {
        return disp->handleRequest (body, clientIP, clientPort);
    });
}

MCPServer::~MCPServer()
{
    stop();
}

bool MCPServer::start (int port, bool loopbackOnly)
{
    if (isRunning())
        return true;

    return transport->start (port, loopbackOnly);
}

void MCPServer::stop()
{
    if (transport != nullptr)
        transport->stop();
}

void MCPServer::runOSCQueryAudit (const juce::String& url)
{
    if (url.isEmpty())
    {
        // Caller (e.g. NetworkTab) signalled OSCQuery is no longer running;
        // stop any in-flight audit.
        oscQueryAuditor.reset();
        return;
    }

    // Recreate the auditor each time so a re-run after an OSCQuery restart
    // picks up the new URL cleanly. The previous instance's destructor
    // joins its thread (3 s timeout) so this is safe even if a prior
    // audit is still in flight — uncommon, since one audit is ~1.5 s end
    // to end.
    oscQueryAuditor = std::make_unique<MCPOSCQueryAuditor>(*mcpLogger,
                                                            generatedToolsJsonPath,
                                                            url);
    oscQueryAuditor->runAudit();
}

} // namespace WFSNetwork
