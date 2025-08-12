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
};
} // namespace audio_plugin