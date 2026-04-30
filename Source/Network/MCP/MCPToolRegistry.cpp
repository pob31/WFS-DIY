#include "MCPToolRegistry.h"

namespace WFSNetwork
{

void MCPToolRegistry::registerTool (ToolDescriptor descriptor)
{
    jassert (descriptor.name.isNotEmpty());
    jassert (descriptor.handler != nullptr);

    // Replace if name already exists (lets callers re-register during reload).
    for (auto& t : tools)
    {
        if (t.name == descriptor.name)
        {
            t = std::move (descriptor);
            return;
        }
    }

    tools.push_back (std::move (descriptor));
}

const ToolDescriptor* MCPToolRegistry::find (const juce::String& name) const
{
    for (const auto& t : tools)
        if (t.name == name)
            return &t;
    return nullptr;
}

} // namespace WFSNetwork
