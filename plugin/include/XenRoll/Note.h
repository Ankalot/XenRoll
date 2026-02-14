#pragma once

namespace audio_plugin {
// Maybe use this?:
// using uchar = unsigned char;
// using ushort = unsigned short;
// uchar octave
// ushort cents
// double time
// double duration
// uchar velocity
// bool isSelected

/**
 * @brief Represents a musical note in microtonal context
 */
struct Note {
    int octave;              ///< Octave number (0+)
    int cents;               ///< Cents within octave (0-1199)
    float time;              ///< Start time in bars
    bool isSelected = false; ///< Whether note is selected in UI
    float duration;          ///< Duration in bars
    float velocity;          ///< MIDI velocity
    int bend = 0;            ///< Pitch bend in cents

    bool operator==(const Note &other) const {
        return octave == other.octave && cents == other.cents && time == other.time &&
               duration == other.duration && velocity == other.velocity && bend == other.bend;
    }
};
} // namespace audio_plugin