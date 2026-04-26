#include "MCPChangeRecords.h"

namespace WFSNetwork
{

MCPChangeRecordBuffer::MCPChangeRecordBuffer (int capacity)
    : cap (juce::jmax (1, capacity))
{
}

void MCPChangeRecordBuffer::push (ChangeRecord record)
{
    const juce::ScopedLock sl (lock);

    if (static_cast<int> (records.size()) >= cap)
        records.pop_front();

    records.push_back (std::move (record));
}

bool MCPChangeRecordBuffer::popBack (ChangeRecord& out)
{
    const juce::ScopedLock sl (lock);
    if (records.empty())
        return false;
    out = std::move (records.back());
    records.pop_back();
    return true;
}

bool MCPChangeRecordBuffer::peekAt (int index, ChangeRecord& out) const
{
    const juce::ScopedLock sl (lock);
    if (index < 0 || index >= static_cast<int> (records.size()))
        return false;
    out = records[static_cast<size_t> (index)];
    return true;
}

bool MCPChangeRecordBuffer::removeAt (int index, ChangeRecord& out)
{
    const juce::ScopedLock sl (lock);
    if (index < 0 || index >= static_cast<int> (records.size()))
        return false;
    auto it = records.begin() + index;
    out = std::move (*it);
    records.erase (it);
    return true;
}

std::vector<ChangeRecord> MCPChangeRecordBuffer::getRecent (int n) const
{
    const juce::ScopedLock sl (lock);

    if (n < 0 || n >= static_cast<int> (records.size()))
        return std::vector<ChangeRecord> (records.begin(), records.end());

    return std::vector<ChangeRecord> (records.end() - n, records.end());
}

int MCPChangeRecordBuffer::size() const noexcept
{
    const juce::ScopedLock sl (lock);
    return static_cast<int> (records.size());
}

void MCPChangeRecordBuffer::clear()
{
    const juce::ScopedLock sl (lock);
    records.clear();
}

int MCPChangeRecordBuffer::markMatchingAsSelfCorrected (std::function<bool (const ChangeRecord&)> predicate)
{
    const juce::ScopedLock sl (lock);
    int count = 0;
    for (auto& rec : records)
    {
        if (predicate (rec))
        {
            rec.isSelfCorrected = true;
            ++count;
        }
    }
    return count;
}

} // namespace WFSNetwork
