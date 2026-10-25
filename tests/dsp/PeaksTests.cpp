#include "giclee/dsp/Peaks.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

using giclee::dsp::computeMinMaxPeaks;
using giclee::dsp::MinMax;

TEST_CASE ("peaks of an evenly divisible buffer")
{
    const std::vector<float> s { 0.1f, -0.2f,   0.5f, 0.3f,   -0.9f, -0.1f };
    std::vector<MinMax> bins (3);
    computeMinMaxPeaks (s, bins);

    CHECK (bins[0].min == -0.2f);
    CHECK (bins[0].max == 0.1f);
    CHECK (bins[1].min == 0.3f);
    CHECK (bins[1].max == 0.5f);
    CHECK (bins[2].min == -0.9f);
    CHECK (bins[2].max == -0.1f);
}

TEST_CASE ("every sample lands in exactly one bin when sizes don't divide")
{
    // A single spike must show up in exactly one bin, wherever it is.
    constexpr size_t numSamples = 1003;
    constexpr size_t numBins = 97;

    for (size_t spike = 0; spike < numSamples; spike += 7)
    {
        std::vector<float> s (numSamples, 0.0f);
        s[spike] = 1.0f;

        std::vector<MinMax> bins (numBins);
        computeMinMaxPeaks (s, bins);

        const auto hits = std::count_if (bins.begin(), bins.end(), [] (const MinMax& b) { return b.max == 1.0f; });
        CHECK (hits == 1);
    }
}

TEST_CASE ("more bins than samples holds values")
{
    const std::vector<float> s { 0.5f, -0.5f };
    std::vector<MinMax> bins (5);
    computeMinMaxPeaks (s, bins);

    for (const auto& b : bins)
        CHECK ((b.max == 0.5f || b.max == -0.5f));

    CHECK (bins.front().max == 0.5f);
    CHECK (bins.back().max == -0.5f);
}

TEST_CASE ("empty input gives zeroed bins; empty output is a no-op")
{
    std::vector<MinMax> bins (4, { 1.0f, 1.0f });
    computeMinMaxPeaks ({}, bins);

    for (const auto& b : bins)
    {
        CHECK (b.min == 0.0f);
        CHECK (b.max == 0.0f);
    }

    const std::vector<float> s { 1.0f };
    computeMinMaxPeaks (s, {});
}
