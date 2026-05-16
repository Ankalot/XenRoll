#pragma once

#include <cmath>
#include <numeric>
#include <utility>

namespace audio_plugin {
class Parameters;

struct RatioMark {
  public:
    float time; ///< Time position in bars

    /**
     * @brief Construct a RatioMark
     * @param lktc Lower key total cents
     * @param hktc Higher key total cents
     * @param t Time position in bars
     * @param ps Pointer to Parameters object
     * @param lni Lower note index (-1 if doesn't exist)
     * @param hni Higher note index (-1 if doesn't exist)
     */
    RatioMark(int lktc, int hktc, float t, Parameters *ps, int lni = -1, int hni = -1)
        : lowerKeyTotalCents(lktc), higherKeyTotalCents(hktc), time(t), params(ps),
          lowerNoteIndex(lni), higherNoteIndex(hni) {
        calculateRatioAndError();
    }

    int getLowerKeyTotalCents() const { return lowerKeyTotalCents; }
    int getHigherKeyTotalCents() const { return higherKeyTotalCents; }
    int getLowerNoteIndex() const { return lowerNoteIndex; }
    int getHigherNoteIndex() const { return higherNoteIndex; }

    /**
     * @brief Get the ratio as a pair (numerator, denominator)
     * @return Pair of integers representing the ratio
     */
    std::pair<int, int> getRatio() const { return std::make_pair(num, den); }

    /**
     * @brief Get the error in cents
     * @return Error value in cents
     */
    int getError() const { return err; }

    /**
     * @brief Set new lowerKeyTotalCents and higherKeyTotalCents.
     * @note If you change both keyTotalCents, you must call this function ONCE after the changes.
     * @param newLowerKeyTotalCents MUST correspond to lowerNoteIndex (if exists)!
     * @param newHigherKeyTotalCents MUST correspond to higherNoteIndex (if exists)!
     */
    void setKeysTotalCents(int newLowerKeyTotalCents, int newHigherKeyTotalCents) {
        lowerKeyTotalCents = newLowerKeyTotalCents;
        higherKeyTotalCents = newHigherKeyTotalCents;
        if (lowerKeyTotalCents > higherKeyTotalCents) {
            std::swap(lowerKeyTotalCents, higherKeyTotalCents);
            std::swap(lowerNoteIndex, higherNoteIndex);
        }
        calculateRatioAndError();
    }

    void setLowerNoteIndex(int newLowerNoteIndex) { lowerNoteIndex = newLowerNoteIndex; }

    void setHigherNoteIndex(int newHigherNoteIndex) { higherNoteIndex = newHigherNoteIndex; }

    void calculateRatioAndError();

  private:
    Parameters *params;
    int lowerNoteIndex, higherNoteIndex; ///< -1 if doesn't exist
    int lowerKeyTotalCents, higherKeyTotalCents;
    int num, den;
    int err; ///< in cents
};
} // namespace audio_plugin