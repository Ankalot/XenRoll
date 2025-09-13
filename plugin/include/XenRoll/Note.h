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

struct Note {
    int octave;
    int cents;
    float time; // in bars
    bool isSelected = false;
    float duration; // in bars
    float velocity;
    int bend = 0; // in cents

    bool operator==(const Note& other) const {
        return octave == other.octave &&
               cents == other.cents &&
               time == other.time &&
               duration == other.duration &&
               velocity == other.velocity &&
               bend == other.bend;
    }
};
} // namespace audio_plugin