#pragma once

#include <JuceHeader.h>
#include "../MCPToolRegistry.h"
#include "../../../Parameters/WFSFileManager.h"

namespace WFSNetwork::Tools::Snapshot
{

inline juce::var buildListSchema()
{
    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (new juce::DynamicObject()));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

/** snapshot.list — read-only enumeration of saved input snapshot names. */
inline ToolResult list (WFSFileManager& fileManager)
{
    juce::StringArray names = fileManager.getInputSnapshotNames();

    juce::Array<juce::var> snapshots;
    for (const auto& n : names)
        snapshots.add (juce::var (n));

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("snapshots", juce::var (snapshots));
    result->setProperty ("count", snapshots.size());
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describe (WFSFileManager& fileManager)
{
    ToolDescriptor d;
    d.name        = "snapshot_list";
    d.description = "List the names of all saved input snapshots in the "
                    "current project folder. Use before snapshot.load to "
                    "validate the name an operator/AI is asking about.";
    d.inputSchema   = buildListSchema();
    d.modifiesState = false;
    d.handler = [&fileManager] (const juce::var&, ChangeRecord*) -> ToolResult
    {
        return list (fileManager);
    };
    return d;
}

} // namespace WFSNetwork::Tools::Snapshot
