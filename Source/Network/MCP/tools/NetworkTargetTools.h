#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "ToolArgs.h"

/*
    The Network tab's target table over MCP: list, add, remove. A fresh session
    has no targets, so every generated network_set_t_s* tool failed with "No
    network target N" until someone pressed Add in the GUI.

    target_id is 1..6, the Network tab's row; it is the target's stored slot,
    not its child position (WFSValueTreeState::getNetworkTargetState). The tab
    applies an MCP write to the live OSC engine itself (NetworkTab's external
    apply), so these tools only change the tree.
*/
namespace WFSNetwork::Tools::NetworkTarget
{

inline juce::var describeTarget (const juce::ValueTree& t, int slot)
{
    using namespace WFSParameterIDs;
    static const juce::StringArray protocols { "DISABLED", "OSC", "Remote", "ADM-OSC", "QLab" };
    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty ("target_id", slot + 1);
    obj->setProperty ("name",      t.getProperty (networkTSname).toString());
    obj->setProperty ("ip",        t.getProperty (networkTSip).toString());
    obj->setProperty ("port",      t.getProperty (networkTSport));
    const int protocol = static_cast<int> (t.getProperty (networkTSProtocol));
    obj->setProperty ("protocol",  protocol);
    if (juce::isPositiveAndBelow (protocol, protocols.size()))
        obj->setProperty ("protocol_name", protocols[protocol]);
    obj->setProperty ("data_mode", t.getProperty (networkTSdataMode));
    obj->setProperty ("rx_enable", static_cast<int> (t.getProperty (networkTSrxEnable)) != 0);
    obj->setProperty ("tx_enable", static_cast<int> (t.getProperty (networkTStxEnable)) != 0);
    if (t.hasProperty (networkTSqlabPatch))
        obj->setProperty ("qlab_patch", t.getProperty (networkTSqlabPatch));
    return juce::var (obj.release());
}

inline juce::var listTargets (WFSValueTreeState& state)
{
    juce::Array<juce::var> out;
    for (int slot = 0; slot < WFSParameterDefaults::maxNetworkTargets; ++slot)
        if (auto t = state.getNetworkTargetState (slot); t.isValid())
            out.add (describeTarget (t, slot));
    return juce::var (out);
}

inline ToolDescriptor describeList (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "network_targets_list";
    d.description = "List the Network tab's targets (up to 6): target_id (the row, 1-6), name, IP, "
                    "port, protocol (0 DISABLED, 1 OSC, 2 Remote, 3 ADM-OSC, 4 QLab, and the newer "
                    "ones by number), data mode, Rx/Tx switches and QLab patch. The "
                    "network_set_t_s* tools address these target_ids. Read-only.";
    d.inputSchema   = Args::schemaOf (std::make_unique<juce::DynamicObject>(), {});
    d.modifiesState = false;
    d.tier          = 1;
    d.handler = [&state] (const juce::var&, ChangeRecord*) -> ToolResult
    {
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("targets", listTargets (state));
        result->setProperty ("max", WFSParameterDefaults::maxNetworkTargets);
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

inline ToolDescriptor describeCreate (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "network_target_create";
    d.description = juce::String (
        "Add a network target in the first free row of the Network tab (up to 6) and return its "
        "target_id. It starts DISABLED with Rx and Tx off, so nothing is sent until you set its "
        "IP, port and protocol with the network_set_t_s* tools; each change applies at once. Not "
        "undoable: delete it instead.") + juce::String (kTier2DescriptionSuffix);
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), {});
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state] (const juce::var&, ChangeRecord* record) -> ToolResult
    {
        const int slot = state.addNetworkTarget();
        if (slot < 0)
            return ToolResult::error ("at_capacity", "All " + juce::String (WFSParameterDefaults::maxNetworkTargets)
                                                       + " network targets are in use");
        if (record != nullptr)
        {
            record->operatorDescription = "Added network target " + juce::String (slot + 1)
                                        + " (structural change - not undoable)";
            record->affectedParameters.add (WFSParameterIDs::NetworkTarget.toString());
            record->affectedGroups.push_back ({ slot + 1, juce::String ("Network") });
            record->undoable = false;
        }
        return ToolResult::ok (describeTarget (state.getNetworkTargetState (slot), slot));
    };
    return d;
}

inline ToolDescriptor describeDelete (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "network_target_delete";
    d.description = juce::String (
        "Remove one network target by its target_id (the Network tab's row, 1-6); the others keep "
        "their rows. Sending to and receiving from it stop at once. Not undoable.")
        + juce::String (kTier2DescriptionSuffix);
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("target_id", Args::integerArg ("Target row, 1-6.", 1, WFSParameterDefaults::maxNetworkTargets));
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "target_id" });
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        if (obj == nullptr || Args::missing (*obj, "target_id"))
            return ToolResult::error ("invalid_args", "Missing required arg: target_id");
        juce::String code, message;
        const auto id = Args::wholeNumber (obj->getProperty ("target_id"), "target_id", 1,
                                           WFSParameterDefaults::maxNetworkTargets, code, message);
        if (! id.has_value())
            return ToolResult::error (code, message);

        const auto name = state.getNetworkTargetState (*id - 1).getProperty (WFSParameterIDs::networkTSname).toString();
        if (! state.removeNetworkTarget (*id - 1))
            return ToolResult::error ("invalid_args", "No network target " + juce::String (*id));

        if (record != nullptr)
        {
            record->operatorDescription = "Removed network target " + juce::String (*id)
                                        + (name.isNotEmpty() ? " (" + name + ")" : juce::String())
                                        + " (structural change - not undoable)";
            record->affectedParameters.add (WFSParameterIDs::NetworkTarget.toString());
            record->affectedGroups.push_back ({ *id, juce::String ("Network") });
            record->undoable = false;
        }
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("deleted_target_id", *id);
        result->setProperty ("targets", listTargets (state));
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

} // namespace WFSNetwork::Tools::NetworkTarget
