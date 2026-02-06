#include "XenRoll/RatioMark.h"
#include "XenRoll/Parameters.h"

namespace audio_plugin {
void RatioMark::calculateRatioAndError() {
    int dcents = higherKeyTotalCents - lowerKeyTotalCents;
    float ratio = std::pow(2.0, dcents / 1200.0);

    float bestError = 1e6;
    int bestNum = 1;
    int bestDen = 1;

    // Search for best rational approximation
    for (int d = 1; d <= params->maxDenRatiosMarks; ++d) {
        // Calculate numerator that gives closest ratio
        int n = static_cast<int>(std::round(ratio * d));

        if (n >= 1) {
            float actualRatio = static_cast<float>(n) / d;
            float error = std::log2(actualRatio / ratio) * 1200.0;

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
}