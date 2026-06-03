#pragma once

#include "XenRoll/data/Note.h"
#include <utility>
#include <vector>

namespace audio_plugin {
class Parameters;

struct RatioMark {
  public:
    /**
     * @brief Construct a RatioMark
     * @param pitch1 First pitch in total cents
     * @param pitch2 Second pitch in total cents
     * @param t Time position in bars
     * @param params Pointer to Parameters object
     * @param noteInd1 First note index (-1 if doesn't exist)
     * @param noteInd2 Second note index (-1 if doesn't exist)
     * @param notes Notes vector. Optional, if provided: pitch1 and pitch2 will be
     * updated according to noteInd1 and noteInd2 (if not -1)
     */
    RatioMark(int pitch1, int pitch2, float t, Parameters *params, int noteInd1 = -1,
              int noteInd2 = -1, const std::vector<Note> &notes = {})
        : pitch1(pitch1), pitch2(pitch2), time(t), params(params), noteInd1(noteInd1),
          noteInd2(noteInd2) {
        if (notes.size() != 0) {
            updatePitchesBasedOnNotes(notes);
        }
        calculateRatioAndError();
    }

    ///< Returns {pitch1, pitch2} in total cents
    std::pair<int, int> getPitches() const { return {pitch1, pitch2}; }
    ///< Returns note's indexes that correspond to {pitch1, pitch2} respectively (-1 => no note)
    std::pair<int, int> getNoteInds() const { return {noteInd1, noteInd2}; }
    ///< Get the ratio as a pair (numerator, denominator)
    std::pair<int, int> getRatio() const { return std::make_pair(num, den); }
    ///< Get the error value in cents
    int getError() const { return err; }
    ///< Get time position in bars
    float getTime() const { return time; }

    ///< Set first and second pitches in total cents. Ratio and error will be recalculated.
    void setPitches(int newPitch1, int newPitch2) {
        pitch1 = newPitch1;
        pitch2 = newPitch2;
        calculateRatioAndError();
    }
    ///< Update pitches & ratio and error (if this ratio mark depends on note(s) & if needed).
    void updatePitchesBasedOnNotes(const std::vector<Note> &notes);
    ///< Set note indexes for pitches. Updates pitches & ratio and error (if needed).
    void setNoteInds(int newNoteInd1, int newNoteInd2, const std::vector<Note> &notes);
    ///< Set time position in bars. Updates pitches & ratio and error (if needed).
    void setTime(float t, const std::vector<Note> &notes);

    ///< Recalculate ratio and error. Call this from outside the ratio mark if params have changed.
    void calculateRatioAndError();

  private:
    Parameters *params;
    float time;             ///< Time position in bars
    int noteInd1, noteInd2; ///< -1 if doesn't exist
    int pitch1, pitch2;
    int num, den;
    int err; ///< in cents
};
} // namespace audio_plugin