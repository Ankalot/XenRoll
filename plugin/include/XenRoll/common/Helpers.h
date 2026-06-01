#pragma once

#include <algorithm>
#include <cmath>
#include <juce_core/juce_core.h>
#include <limits>
#include <set>

namespace audio_plugin {

/**
 * @brief Finds the nearest valid absolute key (totalCents) to the given totalCents.
 *
 * @note totalCents must be >= 0
 * @note keys set must not be empty
 * @param totalCents absolute pitch, octave * 1200 + cents
 * @param keys Set of keys in cents in range [0, 1200)
 * @param numOctaves Total number of octaves available
 * @return The nearest valid total cents value
 */
inline int findNearestKeyTotalCents(int totalCents, const std::set<int> &keys, int numOctaves) {
    int octave = totalCents / 1200;
    int cents = totalCents % 1200;

    auto it = keys.lower_bound(cents);

    int bestTotalCents = 0;
    int minDiff = std::numeric_limits<int>::max();

    auto checkCandidate = [&](int candOctave, int candCents) {
        int candTotalCents = candOctave * 1200 + candCents;
        int diff = std::abs(totalCents - candTotalCents);
        if (diff < minDiff) {
            minDiff = diff;
            bestTotalCents = candTotalCents;
        }
    };

    // Check eq or higher key in this octave
    if (it != keys.end()) {
        checkCandidate(octave, *it);
    }

    // Check lower key in this octave
    if (it != keys.begin()) {
        checkCandidate(octave, *std::prev(it));
    }

    // Check key in higher octave
    if (octave < (numOctaves - 1)) {
        checkCandidate(octave + 1, *keys.begin());
    }

    // Check key in lower octave
    if (octave > 0) {
        checkCandidate(octave - 1, *keys.rbegin());
    }

    return bestTotalCents;
}

/**
 * @brief Converts y-coordinate of panel to totalCents
 * @param y y-coordinate of panel
 * @param numOctaves Total number of octaves available
 * @param octave_height_px Height of octave in pixels
 * @return totalCents of pitch (octave * 1200 + cents) in range [0; num_octaves * 1200 - 1]
 */
inline int yToTotalCents(int y, int num_octaves, float octave_height_px) {
    int totalCents = juce::roundToInt(1200.0f * (num_octaves - y / octave_height_px));
    totalCents = juce::jlimit(0, num_octaves * 1200 - 1, totalCents);

    return totalCents;
}
} // namespace audio_plugin