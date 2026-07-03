#include "TreeParameterStore.h"

#include <algorithm>

namespace spatcore::control::state
{

//==============================================================================
// Construction / Destruction
//==============================================================================

TreeParameterStore::TreeParameterStore (int numUndoDomains, const juce::StringArray& domainNames)
    : undoDomainNames (domainNames)
{
    jassert (numUndoDomains >= 1);
    undoManagers.reserve (static_cast<size_t> (numUndoDomains));
    for (int i = 0; i < numUndoDomains; ++i)
        undoManagers.push_back (std::make_unique<juce::UndoManager>());
}

TreeParameterStore::~TreeParameterStore() = default;

//==============================================================================
// Parameter Access - Type Safe
//==============================================================================

float TreeParameterStore::getFloatParameter (const juce::Identifier& paramId, int channelIndex) const
{
    auto tree = getTreeForParameter (paramId, channelIndex);
    if (tree.isValid() && tree.hasProperty (paramId))
        return static_cast<float> (tree.getProperty (paramId));
    return 0.0f;
}

int TreeParameterStore::getIntParameter (const juce::Identifier& paramId, int channelIndex) const
{
    auto tree = getTreeForParameter (paramId, channelIndex);
    if (tree.isValid() && tree.hasProperty (paramId))
        return static_cast<int> (tree.getProperty (paramId));
    return 0;
}

juce::String TreeParameterStore::getStringParameter (const juce::Identifier& paramId, int channelIndex) const
{
    auto tree = getTreeForParameter (paramId, channelIndex);
    if (tree.isValid() && tree.hasProperty (paramId))
        return tree.getProperty (paramId).toString();
    return {};
}

juce::var TreeParameterStore::getParameter (const juce::Identifier& paramId, int channelIndex) const
{
    auto tree = getTreeForParameter (paramId, channelIndex);
    if (tree.isValid())
        return tree.getProperty (paramId);
    return {};
}

void TreeParameterStore::setParameter (const juce::Identifier& paramId, const juce::var& value, int channelIndex)
{
    auto tree = getTreeForParameter (paramId, channelIndex);
    if (tree.isValid())
        tree.setProperty (paramId, value, getActiveUndoManager());
}

void TreeParameterStore::setParameterWithoutUndo (const juce::Identifier& paramId, const juce::var& value, int channelIndex)
{
    auto tree = getTreeForParameter (paramId, channelIndex);
    if (tree.isValid())
        tree.setProperty (paramId, value, nullptr);
}

//==============================================================================
// Undo / Redo  (per-domain)
//==============================================================================

void TreeParameterStore::setActiveDomain (int domain)
{
    activeDomain = domain;
}

int TreeParameterStore::getActiveDomain() const
{
    return activeDomain;
}

juce::UndoManager* TreeParameterStore::getUndoManagerForDomain (int domain)
{
    jassert (domain >= 0 && domain < static_cast<int> (undoManagers.size()));
    return undoManagers[static_cast<size_t> (domain)].get();
}

juce::UndoManager* TreeParameterStore::getActiveUndoManager()
{
    // MCP-origin writes bypass the JUCE UndoManager entirely — AI changes
    // have a dedicated undo channel (the core MCP undo engine + the app's
    // AI-undo surface). Without this, all AI writes pile into one open JUCE
    // transaction and a single Ctrl+Z reverts every AI change at once.
    if (osc::getCurrentOriginTag() == osc::OriginTag::MCP)
        return nullptr;

    return getUndoManagerForDomain (activeDomain);
}

bool TreeParameterStore::undo()
{
    return getActiveUndoManager()->undo();
}

bool TreeParameterStore::redo()
{
    return getActiveUndoManager()->redo();
}

bool TreeParameterStore::canUndo() const
{
    return undoManagers[static_cast<size_t> (activeDomain)]->canUndo();
}

bool TreeParameterStore::canRedo() const
{
    return undoManagers[static_cast<size_t> (activeDomain)]->canRedo();
}

void TreeParameterStore::beginUndoTransaction (const juce::String& transactionName)
{
    // MCP-origin writes bypass the JUCE UndoManager (see getActiveUndoManager
    // — they get their own undo path through the MCP undo engine).
    // beginNewTransaction doesn't handle nullptr, while setProperty(..., nullptr)
    // does. So when a structural write happens under an MCP origin, we silently
    // skip the transaction-naming step instead of crashing.
    if (auto* mgr = getActiveUndoManager())
        mgr->beginNewTransaction (transactionName);
}

void TreeParameterStore::clearUndoHistory()
{
    getActiveUndoManager()->clearUndoHistory();
}

void TreeParameterStore::clearAllUndoHistories()
{
    for (auto& mgr : undoManagers)
        mgr->clearUndoHistory();
}

//==============================================================================
// Listener Management
//==============================================================================

void TreeParameterStore::addParameterListener (const juce::Identifier& paramId,
                                               ParameterCallback callback,
                                               int channelIndex)
{
    juce::ScopedLock lock (listenerLock);
    parameterListeners.push_back ({ paramId, channelIndex, std::move (callback) });
}

void TreeParameterStore::removeParameterListeners (const juce::Identifier& paramId, int channelIndex)
{
    juce::ScopedLock lock (listenerLock);
    parameterListeners.erase (
        std::remove_if (parameterListeners.begin(), parameterListeners.end(),
            [&] (const ListenerEntry& entry)
            {
                return entry.parameterId == paramId && entry.channelIndex == channelIndex;
            }),
        parameterListeners.end());
}

void TreeParameterStore::addListener (juce::ValueTree::Listener* listener)
{
    state.addListener (listener);
}

void TreeParameterStore::removeListener (juce::ValueTree::Listener* listener)
{
    state.removeListener (listener);
}

void TreeParameterStore::notifyParameterListeners (const juce::Identifier& paramId,
                                                   const juce::var& value,
                                                   int channelIndex)
{
    juce::ScopedLock lock (listenerLock);

    for (const auto& entry : parameterListeners)
    {
        if (entry.parameterId == paramId &&
            (entry.channelIndex == -1 || entry.channelIndex == channelIndex))
        {
            entry.callback (value);
        }
    }
}

//==============================================================================
// Subclass Seams (defaults)
//==============================================================================

int TreeParameterStore::resolveChannelIndex (const juce::ValueTree&) const
{
    return -1;
}

void TreeParameterStore::handlePostWrite (juce::ValueTree&, const juce::Identifier&,
                                          const juce::var&, int)
{
    // Default: no invariants. The app subclass overrides this.
}

//==============================================================================
// ValueTree::Listener Implementation
//==============================================================================

void TreeParameterStore::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                                   const juce::Identifier& property)
{
    const int channelIndex = resolveChannelIndex (treeWhosePropertyHasChanged);

    auto value = treeWhosePropertyHasChanged.getProperty (property);

    // POST-WRITE HOOK: subclass invariants run here, before listener dispatch.
    handlePostWrite (treeWhosePropertyHasChanged, property, value, channelIndex);

    notifyParameterListeners (property, value, channelIndex);
}

void TreeParameterStore::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&)
{
    // Could notify listeners of structural changes if needed
}

void TreeParameterStore::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int)
{
    // Could notify listeners of structural changes if needed
}

void TreeParameterStore::valueTreeChildOrderChanged (juce::ValueTree&, int, int)
{
    // Not typically needed for parameters
}

void TreeParameterStore::valueTreeParentChanged (juce::ValueTree&)
{
    // Not typically needed for parameters
}

} // namespace spatcore::control::state
