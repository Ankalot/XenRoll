#include "XenRoll/data/RatioMark.h"
#include "XenRoll/data/Parameters.h"
#include <cmath>
#include <juce_core/juce_core.h>
#include <numeric>

namespace audio_plugin {
void RatioMark::updatePitchesBasedOnNotes(const std::vector<Note> &notes) {
    bool changed = false;

    if ((noteInd1 > -1) && (noteInd1 < notes.size())) {
        const Note &note = notes[noteInd1];
        int newPitch1 = note.octave * 1200 + note.cents;
        if (notes[noteInd1].bend != 0) {
            if (time > (note.time + note.duration)) {
                newPitch1 += note.bend;
            } else if (time > note.time) {
                newPitch1 += juce::roundToInt(note.bend * (time - note.time) / note.duration);
            }
        }
        if (newPitch1 != pitch1) {
            pitch1 = newPitch1;
            changed = true;
        }
    }

    if ((noteInd2 > -1) && (noteInd2 < notes.size())) {
        const Note &note = notes[noteInd2];
        int newPitch2 = note.octave * 1200 + note.cents;
        if (notes[noteInd2].bend != 0) {
            if (time > (note.time + note.duration)) {
                newPitch2 += note.bend;
            } else if (time > note.time) {
                newPitch2 += juce::roundToInt(note.bend * (time - note.time) / note.duration);
            }
        }
        if (newPitch2 != pitch2) {
            pitch2 = newPitch2;
            changed = true;
        }
    }

    if (changed) {
        calculateRatioAndError();
    }
}

void RatioMark::setNoteInds(int newNoteInd1, int newNoteInd2, const std::vector<Note> &notes) {
    noteInd1 = newNoteInd1;
    noteInd2 = newNoteInd2;
    updatePitchesBasedOnNotes(notes);
}

void RatioMark::setTime(float t, const std::vector<Note> &notes) {
    time = t;
    updatePitchesBasedOnNotes(notes);
}

void RatioMark::calculateRatioAndError() {
    int dcents = std::abs(pitch1 - pitch2);
    float ratio = std::pow(2.0f, dcents / 1200.0f);

    float bestError = 1e6f;
    int bestNum = 1;
    int bestDen = 1;

    // Search for best rational approximation
    for (int d = 1; d <= params->maxDenRatiosMarks; ++d) {
        // Calculate numerator that gives closest ratio
        int n = static_cast<int>(std::round(ratio * d));

        if (n >= 1) {
            float actualRatio = static_cast<float>(n) / d;
            float error = std::log2(actualRatio / ratio) * 1200.0f;

            if (std::abs(error) < std::abs(bestError)) {
                bestError = error;
                bestNum = n;
                bestDen = d;

                // If error is very small, break early
                if (std::abs(bestError) < params->goodEnoughErrorRatiosMarks)
                    break;
            }
        }
    }

    int gcd = std::gcd(bestNum, bestDen);
    num = bestNum / gcd;
    den = bestDen / gcd;
    err = static_cast<int>(std::round(bestError));
}
} // namespace audio_plugin