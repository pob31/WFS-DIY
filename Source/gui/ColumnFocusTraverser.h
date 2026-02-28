#pragma once

#include <JuceHeader.h>

/**
 * ColumnCircuitTraverser
 *
 * Custom keyboard focus traverser that creates independent wrapping circuits
 * per column. Tab cycles within the column of the currently focused component;
 * it never jumps to another column. Invisible or disabled editors are skipped.
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
        for (auto& col : columns)
        {
            for (size_t i = 0; i < col.size(); ++i)
            {
                if (col[i] == current)
                {
                    for (size_t j = 1; j <= col.size(); ++j)
                    {
                        auto* next = col[(i + j) % col.size()];
                        if (next->isVisible() && next->isEnabled())
                            return next;
                    }
                    return current;
                }
            }
        }
        return nullptr;
    }

    juce::Component* getPreviousComponent(juce::Component* current) override
    {
        for (auto& col : columns)
        {
            for (size_t i = 0; i < col.size(); ++i)
            {
                if (col[i] == current)
                {
                    for (size_t j = 1; j <= col.size(); ++j)
                    {
                        size_t prevIdx = (i + col.size() - j) % col.size();
                        auto* prev = col[prevIdx];
                        if (prev->isVisible() && prev->isEnabled())
                            return prev;
                    }
                    return current;
                }
            }
        }
        return nullptr;
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
    std::vector<std::vector<juce::Component*>> columns;
};
