#pragma once

#include <utility>
#include <vector>

namespace audio_plugin {
///< Vector of partials (frequency in Hz, amplitude)
using partialsVec = std::vector<std::pair<float, float>>;

/**
 * @brief Strategy for finding the best analysis window position in PartialsFinder
 */
enum class PartialsFindPosStrat {
    midrangeRMS = 3,         ///< Window that has (minRMS + maxRMS)/2
    medianRMS = 4,           ///< Window that has median RMS
    minRMSfluct = 2,         ///< Window that has min(RMS fluctuation) & RMS in [maxRMS/2; maxRMS]
    maxSpectralFlatness = 5, ///< Window that has max spectral flatness & RMS in [maxRMS/2; maxRMS]
    peakSample = 1           ///< Window that is centered in peak sample
};
} // namespace audio_plugin