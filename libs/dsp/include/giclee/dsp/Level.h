#pragma once

#include <span>

namespace giclee::dsp
{

/** Lowest level reported by gainToDecibels(), in dBFS. */
inline constexpr float kSilenceDb = -144.0f;

/** Largest absolute sample value, or 0 for an empty buffer. */
float peak (std::span<const float> samples) noexcept;

/** Root-mean-square level, or 0 for an empty buffer. */
float rms (std::span<const float> samples) noexcept;

/** Linear gain to decibels, clamped below at floorDb. */
float gainToDecibels (float gain, float floorDb = kSilenceDb) noexcept;

/** Decibels to linear gain. */
float decibelsToGain (float decibels) noexcept;

} // namespace giclee::dsp
