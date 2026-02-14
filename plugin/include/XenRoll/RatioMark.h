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
     */
    RatioMark(int lktc, int hktc, float t, Parameters *ps)
        : lowerKeyTotalCents(lktc), higherKeyTotalCents(hktc), time(t), params(ps) {
        calculateRatioAndError();
    }

    int getLowerKeyTotalCents() const { return lowerKeyTotalCents; }
    int getHigherKeyTotalCents() const { return higherKeyTotalCents; }

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

    void setLowerKeyTotalCents(int newLowerKeyTotalCents) {
        lowerKeyTotalCents = newLowerKeyTotalCents;
        calculateRatioAndError();
    }

    void setHigherKeyTotalCents(int newHigherKeyTotalCents) {
        higherKeyTotalCents = newHigherKeyTotalCents;
        calculateRatioAndError();
    }

    void calculateRatioAndError();

  private:
    Parameters *params;
    int lowerKeyTotalCents, higherKeyTotalCents;
    int num, den;
    int err; ///< in cents
};
} // namespace audio_plugin