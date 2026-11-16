#pragma once

#include <vector>
#include <juce_core/juce_core.h>

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Holds one or more wavetables. Each table is a fixed-size (power of two)
// buffer of samples. Tables can be morphed between. No reallocation after
// construction, so safe to reference from the audio thread.
// -----------------------------------------------------------------------------
class WavetableData
{
public:
    WavetableData() = default;
    ~WavetableData() = default;

    // Allocate numTables tables of tableSize samples (tableSize will be rounded
    // up to a power of two). Existing data is preserved where possible.
    void allocate (int numTables, int tableSize);

    void clear();

    int getNumTables() const noexcept { return static_cast<int> (m_tables.size()); }
    int getTableSize() const noexcept { return m_tableSize; }
    int getTableSizeMask() const noexcept { return m_tableSizeMask; }

    const float* getTable (int tableIndex) const noexcept
    {
        return m_tables[clampIndex (tableIndex)].data();
    }

    float* getTable (int tableIndex) noexcept
    {
        return m_tables[clampIndex (tableIndex)].data();
    }

    void setTable (int tableIndex, const float* source, int numSamples);

private:
    int clampIndex (int index) const noexcept
    {
        return std::clamp (index, 0, static_cast<int> (m_tables.size()) - 1);
    }

    std::vector<std::vector<float>> m_tables;
    int m_tableSize = 0;
    int m_tableSizeMask = 0;
};

} // namespace dsp
} // namespace omnivy
