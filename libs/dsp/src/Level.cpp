#include "giclee/dsp/Level.h"

#include <algorithm>
#include <cmath>

namespace giclee::dsp
{

float peak (std::span<const float> samples) noexcept
{
    float result = 0.0f;

    for (const auto s : samples)
        result = std::max (result, std::abs (s));

    return result;
}

float rms (std::span<const float> samples) noexcept
{
    if (samples.empty())
        return 0.0f;

    // Accumulate in double so long buffers don't lose precision.
    double sum = 0.0;

    for (const auto s : samples)
        sum += static_cast<double> (s) * static_cast<double> (s);

    return static_cast<float> (std::sqrt (sum / static_cast<double> (samples.size())));
}

float gainToDecibels (float gain, float floorDb) noexcept
{
    if (! (gain > 0.0f))
        return floorDb;

    return std::max (floorDb, 20.0f * std::log10 (gain));
}

float decibelsToGain (float decibels) noexcept
{
    return std::pow (10.0f, decibels / 20.0f);
}

} // namespace giclee::dsp
