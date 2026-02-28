#pragma once

#include <JuceHeader.h>

/**
 * ColumnCircuitTraverser
 *
 * Custom keyboard focus traverser that creates independent wrapping circuits
 * per column. Tab cycles within the column of the currently focused component;
 * it never jumps to another column. Invisible or disabled editors are skipped.
 *
 * Also handles editable Labels whose internal TextEditor child receives focus:
 * if the focused component is a child of a circuit member, it is treated as
 * that circuit member for navigation purposes.
 */
class ColumnCircuitTraverser : public juce::ComponentTraverser
{
public:
    explicit ColumnCircuitTraverser(std::vector<std::vector<juce::Component*>> cols)
        : columns(std::move(cols)) {}

    juce::Component* getDefaultComponent(juce::Component*) override
    {
        for (auto& col : columns)
            for (auto* c : col)
                if (c->isVisible() && c->isEnabled())
                    return c;
        return nullptr;
    }

    juce::Component* getNextComponent(juce::Component* current) override
    {
        auto [col, idx] = findInCircuit(current);
        if (col != nullptr)
        {
            for (size_t j = 1; j <= col->size(); ++j)
            {
                auto* next = (*col)[(idx + j) % col->size()];
                if (next->isVisible() && next->isEnabled())
                    return next;
            }
            return current;
        }
        auto* def = getDefaultComponent(nullptr);
        return def != nullptr ? def : current;
    }

    juce::Component* getPreviousComponent(juce::Component* current) override
    {
        auto [col, idx] = findInCircuit(current);
        if (col != nullptr)
        {
            for (size_t j = 1; j <= col->size(); ++j)
            {
                size_t prevIdx = (idx + col->size() - j) % col->size();
                auto* prev = (*col)[prevIdx];
                if (prev->isVisible() && prev->isEnabled())
                    return prev;
            }
            return current;
        }
        auto all = getAllComponents(nullptr);
        return all.empty() ? current : all.back();
    }

    std::vector<juce::Component*> getAllComponents(juce::Component*) override
    {
        std::vector<juce::Component*> all;
        for (auto& col : columns)
            for (auto* c : col)
                if (c->isVisible() && c->isEnabled())
                    all.push_back(c);
        return all;
    }

private:
    /** Find which circuit column and index a component belongs to.
     *  Checks direct match first, then whether current is a child of a
     *  circuit member (handles editable Label internal TextEditors). */
    std::pair<std::vector<juce::Component*>*, size_t> findInCircuit(juce::Component* current)
    {
        // Direct match
        for (auto& col : columns)
            for (size_t i = 0; i < col.size(); ++i)
                if (col[i] == current)
                    return { &col, i };

        // Child-of match (e.g. Label's internal TextEditor)
        for (auto& col : columns)
            for (size_t i = 0; i < col.size(); ++i)
                if (col[i]->isParentOf(current))
                    return { &col, i };

        return { nullptr, 0 };
    }

    std::vector<std::vector<juce::Component*>> columns;
};
