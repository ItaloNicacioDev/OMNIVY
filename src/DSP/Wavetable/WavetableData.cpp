#include "DSP/Wavetable/WavetableData.h"

#include <cmath>
#include <algorithm>

namespace omnivy {
namespace dsp {

void WavetableData::allocate (int numTables, int tableSize)
{
    numTables = std::max (1, numTables);

    // Round table size up to a power of two.
    int size = 1;
    while (size < tableSize) size <<= 1;
    size = std::max (size, 16);

    m_tables.assign (static_cast<size_t> (numTables), std::vector<float> (size, 0.0f));
    m_tableSize = size;
    m_tableSizeMask = size - 1;
}

void WavetableData::clear()
{
    for (auto& table : m_tables)
        std::fill (table.begin(), table.end(), 0.0f);
}

void WavetableData::setTable (int tableIndex, const float* source, int numSamples)
{
    if (m_tables.empty() || source == nullptr)
        return;

    auto& table = getTable (tableIndex);
    const int n = std::min (numSamples, m_tableSize);
    std::copy (source, source + n, table.begin());
    if (n < m_tableSize)
        std::fill (table.begin() + n, table.end(), 0.0f);
}

} // namespace dsp
} // namespace omnivy
