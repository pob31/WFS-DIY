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
#include "tools/GetParameterTool.h"
#include "tools/DescribeParametersTool.h"
#include "tools/StateInspectionTools.h"
#include "tools/StateDeltaTool.h"
#include "tools/ChannelLifecycleTools.h"
#include "tools/ReverbAutoLayoutTool.h"

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

    // Server identity for the MCP `initialize` response. These are the
    // literals the core dispatcher used to hard-code — moved here verbatim
    // (byte-identical wire output) when the dispatcher went app-agnostic.
    ServerIdentity identity;
    identity.name    = "WFS-DIY";
    identity.version = juce::String (ProjectInfo::versionString);
    // Welcome / quick-start surfaced to the model as system context on
    // connect, so an AI lands oriented instead of having to discover the
    // surface from scratch. ASCII only.
    identity.instructions =
        "Welcome to WFS-DIY, a Wave Field Synthesis spatial audio app. "
        "First reads to orient yourself: "
        "(1) `mcp_describe_parameters` is the source of truth for every "
        "writable parameter - filter by `prefix`, `scope` "
        "(global/input/output/reverb/cluster/eq_band), `group_key`, or "
        "`domain` (wfs_synthesis / reverb / binaural / adm_osc / "
        "floor_reflections / live_source / tracking / routing / network / "
        "visualisation_only / metadata). Always check this before "
        "guessing a parameter name. "
        "(2) `session_get_state` for a per-channel id+name+position "
        "summary; `session_get_global_state` for stage / origin / master "
        "/ binaural / network globals (use the `sections` filter to keep "
        "the response small); `session_get_channel_full` for everything "
        "on one channel. "
        "(3) `session_get_state_delta` between turns to notice when "
        "operator UI / OSC / automation changed state under you. "
        "Writing: prefer `wfs_set_parameter_batch` for multi-write flows "
        "(up to 100 atomic writes, single undo entry, single "
        "confirmation handshake). Single writes go through the auto-"
        "generated `<area>_set_<param>` tools; the `wfs_set_parameter` "
        "escape hatch covers anything they miss. Channel lifecycle: "
        "`input_create`/`input_delete`, same for output and reverb. "
        "Reading: `wfs_get_parameter` and `wfs_get_parameters` for "
        "ad-hoc reads matching the write API's shape. "
        "Undo: `mcp_undo_last_ai_change` reverses the latest AI write "
        "(or batch); `mcp_get_ai_change_history(compact=true)` is the "
        "cheap scan; `mcp_redo_last_undone_ai_change` for redo. "
        "Tiers: tier-1 runs immediately; tier-2 needs a confirm token "
        "OR an open operator window (auto-confirm or safety gate); "
        "tier-3 needs the safety gate to be open (which also covers "
        "tier-2 - the gate is the operator's superset trust window). "
        "Both windows auto-close after 5 minutes, operator-only. "
        "If you want a guided workflow (session startup, system "
        "tuning, snapshot management, voice rehearsal, etc.), call "
        "`prompts/list` and fetch the matching template.";

    dispatcher       = std::make_unique<MCPDispatcher> (std::move (identity),
                                                        *registry, *changeRecords,
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
    // Full-session save — the only headless path to persist a project
    // (every other saveCompleteConfig caller is a UI button). Tier 2.
    // Added for the control-replay harnesses (docs/architecture/
    // control-replay-harness.md); useful to any MCP client that wants
    // its changes to survive a restart.
    registry->registerTool (Tools::Session::describeSave (fileMgr));
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

    // High-level placement: classifies speaker topology and writes reverb
    // positions/orientations/pitch in one atomic batch. SDN-aware (chaotic
    // standoff to avoid metallic ringing). See
    // Documentation/MCP/specs/REVERB_AUTO_LAYOUT_*.md for the full spec.
    registry->registerTool (Tools::ReverbAutoLayout::describe (state));

    // Read-side counterparts to the write API. wfs_get_parameter +
    // wfs_get_parameters mirror wfs_set_parameter / batch shapes.
    registry->registerTool (Tools::GetParameter::describeSingle (state));
    registry->registerTool (Tools::GetParameter::describeBatch (state));

    // Read-only registry tool — surfaces every known parameter so the AI
    // can plan writes from the schema instead of guessing. Must come after
    // MCPParameterRegistry::loadFromManifest above.
    registry->registerTool (Tools::DescribeParameters::describeTool());

    // Read-only deep-state tools — globals + per-channel full dump,
    // complementing session_get_state's per-channel summary.
    registry->registerTool (Tools::StateInspection::describeGlobalState (state));
    registry->registerTool (Tools::StateInspection::describeChannelFull (state));

    // Server-wide delta-since-last-call snapshot. Lets the AI notice
    // when state drifted under it (operator UI, OSC, automation, etc.).
    registry->registerTool (Tools::StateDelta::describe (state));

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
