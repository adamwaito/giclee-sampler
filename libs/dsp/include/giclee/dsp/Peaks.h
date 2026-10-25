#pragma once

#include <cstddef>
#include <span>

namespace giclee::dsp
{

/** Minimum and maximum sample value within one waveform display bin. */
struct MinMax
{
    float min = 0.0f;
    float max = 0.0f;
};

/** Reduces samples to bins.size() min/max pairs for waveform drawing.

    Sample i goes to bin floor(i * numBins / numSamples), so every sample lands in exactly
    one bin. With more bins than samples, the empty bins repeat the previous sample so the
    drawn line stays continuous. Does not allocate.
*/
void computeMinMaxPeaks (std::span<const float> samples, std::span<MinMax> bins) noexcept;

} // namespace giclee::dsp
