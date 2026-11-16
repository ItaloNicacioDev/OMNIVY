#pragma once

namespace omnivy {
namespace dsp {

// -----------------------------------------------------------------------------
// Filter type selection.
// -----------------------------------------------------------------------------
enum class FilterType : int
{
    LowPass = 0,
    HighPass,
    BandPass,
    Count
};

// -----------------------------------------------------------------------------
// Abstract filter interface. Real-time safe, no allocation.
// -----------------------------------------------------------------------------
class FilterBase
{
public:
    virtual ~FilterBase() = default;

    virtual void setSampleRate (double sampleRate) = 0;
    virtual void reset() = 0;

    virtual void setType (FilterType type) = 0;
    virtual FilterType getType() const = 0;

    virtual void setCutoff (float frequencyHz) = 0;
    virtual float getCutoff() const = 0;

    virtual void setResonance (float resonance) = 0;
    virtual float getResonance() const = 0;

    // Process one mono sample.
    virtual float processSample (float sample) = 0;
};

} // namespace dsp
} // namespace omnivy
