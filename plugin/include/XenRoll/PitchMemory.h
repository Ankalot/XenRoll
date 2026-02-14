// =====================================================================================
// ========================= Short-term Pitch Memory Algorithm =========================
// =====================================================================================
// Author: Ankalot
//
// Description:
//     Each note leaves a trace in pitch memory. Notes and traces influence each other
//     (such phenomena as consonance, dissonance, stability, tension and tonality can
//     be derived from that). Interactions are determined by the dissonance/discordance
//     metric. Below will be described an algorithm.
//
// Idea inspiration: GrFNN (but there the mechanism is completely different, there are
//     nonlinear oscillators, I was inspired by the pitch memory concept only)
//
// Algorithm:
// 0. Set of parameters:
//    * a in [0; 0.5], default 0.2     - TV_i value for HV_i = 0
//    * b in [0; 1.0], default 0.3     - Additional influence on trace values
//    * c in [0; 0.5], default 0.0     - If TV drops lower than this it becomes 0
// 1. After the start of each note there remains a trace with trace value (TV) in [0; 1]
//    range (TVs change over time, based on other notes). Each note has a harmonicity
//    value (HV) in [-1; 1] range. First note has TV = 1.0 and HV = 1.0.
// 2. We add notes in the order they are played. When i-th note is added:
//    2.1 For each existing trace (let's consider j-th trace):
//        2.1.1 Calculate dissonance value (DV) in [-1; +1] range of interval which is
//              formed by i-th note and a j-th trace (using dissonance metric).
//              Let's call it DV_ij.
//    2.2 HV_i = -SUM_j(TV_j*DV_ij)/SUM_j(TV_j)
//    2.3 TV_i = { (HV_i + 1)*a;    if HV_i <= 0
//               { (1-a)*HV_i + a;  if HV_i > 0
//        if TV_i < c:
//            TV_i := 0
//    2.4 For each existing trace (let's consider j-th trace):
//        2.4.1 Update trace value
//              TV_j = min(1.0, TV_j*2^(min(1.0, TV_i + b)*HV_i))
//              if TV_j < c:
//                  TV_j := 0
//
// TODO:
//    1. chords are sequential notes with dt -> 0, are they taken into account correctly?
//       (NEED RESEARCH)
//    2. add velocity effect?
//    3. add note duration effect?
//    4. what about pitch bends?

#pragma once

#include "DissonanceMeter.h"
#include "Note.h"
#include <atomic>
#include <memory>
#include <optional>
#include <set>

namespace audio_plugin {
/**
 * value - vector containing total сents of all pitches
 * key - map:
 *      key - time
 *      value - vector containing trace values of all pitches
 */
using PitchTraces = std::pair<std::vector<int>, std::map<float, std::vector<float>>>;
///< PitchTraces and Harmonicity values for each note
using PitchMemoryResults = std::pair<PitchTraces, std::vector<float>>;

// Not optimized. At least for now. Not thread safe because it is not required.
class PitchMemory {
  public:
    /**
     * @brief Construct a PitchMemory with algorithm parameters
     * @param dissonanceMeter Shared pointer to DissonanceMeter for calculating dissonance
     * @param TV_val_for_zero_HV Trace value when harmonicity is zero (0-0.5, default 0.2)
     * @param TV_add_influence Additional influence on trace values (0-1, default 0.3)
     * @param TV_min_nonzero Minimum non-zero trace value (0-0.5, default 0.0)
     * @param num_octaves Number of octaves to consider (default 10)
     */
    PitchMemory(std::shared_ptr<DissonanceMeter> dissonanceMeter, float TV_val_for_zero_HV = 0.2f,
                float TV_add_influence = 0.3f, float TV_min_nonzero = 0.0f, int num_octaves = 10);

    /**
     * @brief Find pitch traces formed by notes
     * @param notes Vector of notes
     * @param terminate Atomic flag to terminate the computation early
     * @return Optional pair containing pitch traces and harmonicity values for each note
     */
    std::optional<PitchMemoryResults> findPitchTraces(const std::vector<Note> &notes,
                                                      const std::atomic<bool> &terminate = false);

    /**
     * @brief Find harmonicity values for each key (pitch) based on pitch memory results
     * @param pitchMemoryResults Results from findPitchTraces()
     * @param terminate Atomic flag to terminate the computation early
     * @return Optional map of total cents to harmonicity values
     */
    std::optional<std::map<int, float>>
    findKeysHarmonicity(const PitchMemoryResults &pitchMemoryResults,
                        const std::atomic<bool> &terminate = false);

    /**
     * @brief Set the trace value for zero harmonicity
     * @param new_TV_val_for_zero_HV Value in range [0, 0.5]
     */
    void set_TV_val_for_zero_HV(float new_TV_val_for_zero_HV);

    /**
     * @brief Set the additional influence on trace values
     * @param new_TV_add_influence Value in range [0, 1]
     */
    void set_TV_add_influence(float new_TV_add_influence);

    /**
     * @brief Set the minimum non-zero trace value
     * @param new_TV_min_nonzero Value in range [0, 0.5]
     */
    void set_TV_min_nonzero(float new_TV_min_nonzero);

  private:
    int num_octaves;
    float TV_val_for_zero_HV;
    float TV_add_influence;
    float TV_min_nonzero;
    std::shared_ptr<DissonanceMeter> dissonanceMeter;
};

} // namespace audio_plugin