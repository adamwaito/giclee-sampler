#include "giclee/dsp/Peaks.h"

#include <algorithm>
#include <cstdint>

namespace giclee::dsp
{

void computeMinMaxPeaks (std::span<const float> samples, std::span<MinMax> bins) noexcept
{
    if (bins.empty())
        return;

    if (samples.empty())
    {
        std::fill (bins.begin(), bins.end(), MinMax {});
        return;
    }

    const auto numSamples = static_cast<std::uint64_t> (samples.size());
    const auto numBins = static_cast<std::uint64_t> (bins.size());
    std::uint64_t sampleIndex = 0;

    for (std::uint64_t bin = 0; bin < numBins; ++bin)
    {
        // First sample index of the next bin: ceil((bin + 1) * numSamples / numBins).
        const auto end = ((bin + 1) * numSamples + numBins - 1) / numBins;

        if (sampleIndex >= end)
        {
            // More bins than samples: hold the previous sample.
            const auto s = samples[static_cast<std::size_t> (std::min (sampleIndex, numSamples) - 1)];
            bins[static_cast<std::size_t> (bin)] = { s, s };
            continue;
        }

        auto lo = samples[static_cast<std::size_t> (sampleIndex)];
        auto hi = lo;

        for (++sampleIndex; sampleIndex < end; ++sampleIndex)
        {
            const auto s = samples[static_cast<std::size_t> (sampleIndex)];
            lo = std::min (lo, s);
            hi = std::max (hi, s);
        }

        bins[static_cast<std::size_t> (bin)] = { lo, hi };
    }
}

} // namespace giclee::dsp
