#pragma once

#include <atomic>

namespace audio_plugin {
static constexpr uint64_t INVALID_NOTE_ID = 0;
/**
 * @brief Represents a musical note in microtonal context
 */
struct Note {
    int octave;              ///< Octave number (0+)
    int cents;               ///< Cents within octave (0-1199)
    float time;              ///< Start time in bars
    bool isSelected = false; ///< Whether note is selected in UI
    float duration;          ///< Duration in bars
    float velocity;          ///< Velocity in range [0.0; 1.0]
    int bend = 0;            ///< Pitch bend in cents
    uint64_t id;             ///< ID, not saved via stateInformation (at least for now)

    ///< Helper function to generate IDs
    static uint64_t generateId() {
        static std::atomic<uint64_t> counter{1};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }

    Note() { id = generateId(); }

    Note(int octave, int cents, float time, bool isSelected, float duration, float velocity,
         int bend = 0)
        : octave(octave), cents(cents), time(time), isSelected(isSelected), duration(duration),
          velocity(velocity), bend(bend) {
        id = generateId();
    }

    bool operator==(const Note &other) const {
        // NOTE: IT CHECKS ID TOO!!!
        return id == other.id && octave == other.octave && cents == other.cents &&
               time == other.time && duration == other.duration && velocity == other.velocity &&
               bend == other.bend;
    }
};
} // namespace audio_plugin