#include "giclee/dsp/Level.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <numbers>
#include <vector>

using namespace giclee::dsp;
using Catch::Matchers::WithinAbs;

namespace
{
    std::vector<float> sine (float amplitude, int cycles, int samplesPerCycle)
    {
        std::vector<float> s (static_cast<size_t> (cycles * samplesPerCycle));

        for (size_t i = 0; i < s.size(); ++i)
            s[i] = amplitude * std::sin (2.0f * std::numbers::pi_v<float> * static_cast<float> (i) / static_cast<float> (samplesPerCycle));

        return s;
    }
}

TEST_CASE ("peak and rms of empty buffers are zero")
{
    CHECK (peak ({}) == 0.0f);
    CHECK (rms ({}) == 0.0f);
}

TEST_CASE ("peak finds the largest absolute value")
{
    const std::vector<float> s { 0.1f, -0.7f, 0.5f, 0.0f };
    CHECK (peak (s) == 0.7f);
}

TEST_CASE ("rms of a whole number of sine cycles is amplitude / sqrt 2")
{
    const auto s = sine (0.5f, 10, 480);
    CHECK_THAT (rms (s), WithinAbs (0.5 / std::numbers::sqrt2, 1e-5));
    CHECK_THAT (peak (s), WithinAbs (0.5, 1e-4));
}

TEST_CASE ("rms of a constant is its magnitude")
{
    const std::vector<float> s (1000, -0.25f);
    CHECK_THAT (rms (s), WithinAbs (0.25, 1e-6));
}

TEST_CASE ("decibel conversions")
{
    CHECK_THAT (gainToDecibels (1.0f), WithinAbs (0.0, 1e-6));
    CHECK_THAT (gainToDecibels (0.5f), WithinAbs (-6.0206, 1e-3));
    CHECK_THAT (decibelsToGain (-20.0f), WithinAbs (0.1, 1e-6));

    SECTION ("silence and invalid input clamp to the floor")
    {
        CHECK (gainToDecibels (0.0f) == kSilenceDb);
        CHECK (gainToDecibels (-1.0f) == kSilenceDb);
        CHECK (gainToDecibels (std::nanf ("")) == kSilenceDb);
        CHECK (gainToDecibels (1e-9f, -90.0f) == -90.0f);
    }

    SECTION ("round trip")
    {
        for (const auto db : { -96.0f, -48.0f, -12.5f, 0.0f, 6.0f })
            CHECK_THAT (gainToDecibels (decibelsToGain (db)), WithinAbs (db, 1e-3));
    }
}
