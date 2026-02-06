#pragma once

#include <cmath>
#include <numeric>
#include <utility>

namespace audio_plugin {
class Parameters;

struct RatioMark {
  public:
    float time; // in bars

    RatioMark(int lktc, int hktc, float t, Parameters* ps)
        : lowerKeyTotalCents(lktc), higherKeyTotalCents(hktc), time(t), params(ps) {
        calculateRatioAndError();
    }

    int getLowerKeyTotalCents() const { return lowerKeyTotalCents; }
    int getHigherKeyTotalCents() const { return higherKeyTotalCents; }
    
    std::pair<int, int> getRatio() const { return std::make_pair(num, den); }

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
    Parameters* params;
    int lowerKeyTotalCents, higherKeyTotalCents;
    int num, den;
    int err; // in cents
};
} // namespace audio_plugin