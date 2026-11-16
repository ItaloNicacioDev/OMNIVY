#include "DSP/Wavetable/WavetableGenerator.h"
#include "DSP/Utilities/DSPUtilities.h"

#include <cmath>

namespace omnivy {
namespace dsp {

void WavetableGenerator::generateSine (WavetableData& data, int tableIndex)
{
    const int size = data.getTableSize();
    if (size <= 0) return;
    float* table = data.getTable (tableIndex);
    for (int i = 0; i < size; ++i)
    {
        const double phase = static_cast<double> (i) / static_cast<double> (size);
        table[i] = static_cast<float> (std::sin (TwoPi * phase));
    }
}

void WavetableGenerator::generateSaw (WavetableData& data, int tableIndex, int numHarmonics)
{
    const int size = data.getTableSize();
    if (size <= 0) return;
    float* table = data.getTable (tableIndex);
    numHarmonics = std::max (1, numHarmonics);

    for (int i = 0; i < size; ++i)
    {
        const double phase = static_cast<double> (i) / static_cast<double> (size);
        double sum = 0.0;
        for (int h = 1; h <= numHarmonics; ++h)
            sum += std::sin (TwoPi * h * phase) / static_cast<double> (h);
        // Normalise to approx [-1, 1].
        table[i] = static_cast<float> (sum * (2.0 / Pi));
    }
}

} // namespace dsp
} // namespace omnivy
