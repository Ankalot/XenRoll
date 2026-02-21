#pragma once

#include "PartialsFinder.h"
#include "RatioMark.h"
#include "Theme.h"
#include "Zones.h"
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <stdexcept>

// total cents = 1200*octave + cents
// Ghost notes = notes from other instances of xenroll

namespace audio_plugin {
/**
 * std::vector<float> is for storing time in bars
 * std::vector<int> is for storing pitch in total cents (-1 represents absence of pitch, curve
 * breaks there)
 */
using PitchCurve = std::pair<std::vector<float>, std::vector<int>>;

class Parameters {
  public:
    enum GenNewKeysTactics { DiverseIntervals = 1, Random = 2 };
    static const juce::Array<juce::String> getGenNewKeysTacticsNames() {
        return {"+diverse intervals", "random"};
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~ HOTKEYS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    enum hotkeys {
        timeSnap_withAlt = 'a',
        keySnap_withAlt = 's',
        editRatiosMarks_withAlt = 'd',
        pitchMemory_withAlt = 'f'
    };

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~ Constants ~~~~~~~~~~~~~~~~~~~~~~~~~~~
    static constexpr int num_octaves = 10;
    // ===== Vocal to melody =====
    static constexpr float minVocalVolume_dB = -60.0f;
    static constexpr float maxVocalVolume_dB = 0.0f;
    // ==========================

    // ~~~~~~~~~~~~~~~~~~~~~~~~ Minimum values ~~~~~~~~~~~~~~~~~~~~~~~~
    static constexpr int min_editorWidth = 1420;
    static constexpr int min_editorHeight = 600;
    static constexpr int min_num_bars = 1;
    static constexpr int min_num_beats = 1;
    static constexpr int min_num_subdivs = 1;
    static constexpr int min_start_octave = 0;
    static constexpr double min_A4Freq = 400.0;
    static constexpr float min_noteRectHeightCoef = 0.02f;
    static constexpr int min_num_new_notes = 1;
    static constexpr int min_minDistExistNewKeys = 10;
    static constexpr int min_minDistBetweenNewKeys = 10;
    static constexpr int min_maxDenRatiosMarks = 16;
    static constexpr int min_goodEnoughErrorRatiosMarks = 1;
    // ================== Intellectual ==================
    // Partials/dissonance
    static constexpr int min_plotPartialsTotalCents = 0;
    static constexpr int min_plotDissonanceTotalCents = 0;
    static constexpr float min_findPartialsdBThreshold = -100.0f;
    static constexpr float min_roughCompactFrac = 0.0f;
    static constexpr float min_dissonancePow = 0.5f;
    // Pitch memory
    static constexpr float min_pitchMemoryTVvalForZeroHV = 0.0f;
    static constexpr float min_pitchMemoryTVaddInfluence = 0.0f;
    static constexpr float min_pitchMemoryTVminNonzero = 0.0f;
    // ================= Vocal to melody =================
    static constexpr int min_vocalToMelodyDCents = 10;
    static constexpr float min_micGain_dB = -24.0f;

    // ~~~~~~~~~~~~~~~~~~~~~~~~ Maximum values ~~~~~~~~~~~~~~~~~~~~~~~~
    static constexpr int max_editorWidth = 10000;
    static constexpr int max_editorHeight = 10000;
    static constexpr int max_num_bars = 999;
    static constexpr int max_num_beats = 16;
    static constexpr int max_num_subdivs = 16;
    static constexpr int max_start_octave = 9;
    static constexpr double max_A4Freq = 484.0;
    static constexpr float max_noteRectHeightCoef = 0.1f;
    static constexpr int max_num_new_notes = 10;
    static constexpr int max_minDistExistNewKeys = 100;
    static constexpr int max_minDistBetweenNewKeys = 100;
    static constexpr int max_maxDenRatiosMarks = 64;
    static constexpr int max_goodEnoughErrorRatiosMarks = 20;
    // ================== Intellectual ==================
    // Partials/dissonance
    static constexpr int max_plotPartialsTotalCents = num_octaves * 1200 - 1;
    static constexpr int max_plotDissonanceTotalCents = (num_octaves - 1) * 1200 - 1;
    static constexpr float max_findPartialsdBThreshold = 0.0f;
    static constexpr float max_roughCompactFrac = 1.0f;
    static constexpr float max_dissonancePow = 5.0f;
    // Pitch memory
    static constexpr float max_pitchMemoryTVvalForZeroHV = 0.5f;
    static constexpr float max_pitchMemoryTVaddInfluence = 1.0f;
    static constexpr float max_pitchMemoryTVminNonzero = 0.5f;
    // ================= Vocal to melody =================
    static constexpr int max_vocalToMelodyDcents = 90;
    static constexpr float max_micGain_dB = 24.0f;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~ Saved params ~~~~~~~~~~~~~~~~~~~~~~~~~
    int editorWidth = 1420;
    int editorHeight = 700;
    int num_beats = 4;
    int num_subdivs = 4;
    int start_octave = 2;
    std::atomic<double> A4Freq = 440.0;
    float noteRectHeightCoef = 0.04f;
    Zones zones;
    std::vector<RatioMark> ratiosMarks = {};
    bool autoCorrectRatiosMarks = true;
    int maxDenRatiosMarks = 40;
    int goodEnoughErrorRatiosMarks = 4;
    Theme::ThemeType themeType = Theme::ThemeType::Gray;
    bool playDraggedNotes = true;
    int channelIndex = -1; ///< -1 means uninited state, normal range 0-15
    // ================== Intellectual ==================
    // Partials/dissonance
    std::atomic<int> findPartialsFFTSize = 8192;
    std::atomic<float> findPartialsdBThreshold = -60.0f;
    std::atomic<PartialsFinder::PosFindStrat> findPartialsStrat =
        PartialsFinder::PosFindStrat::maxSpectralFlatness;
    float roughCompactFrac = 0.0f;
    float dissonancePow = 1.0f;
    // Pitch memory
    float pitchMemoryTVvalForZeroHV = 0.2f;
    float pitchMemoryTVaddInfluence = 0.3f;
    float pitchMemoryTVminNonzero = 0.0f;
    bool pitchMemoryShowOnlyHarmonicity = true;
    // ================= Vocal to melody =================
    std::atomic<bool> vocalToMelodyGenCurve = true;
    std::atomic<bool> vocalToMelodyGenNotes = true;
    // Params for vocalToMelodyGenNotes:
    std::atomic<float> vocalToMelodyMinNoteDuration = 1.0f / 128; ///< in bars
    std::atomic<int> vocalToMelodyDCents = 50;
    std::atomic<bool> vocalToMelodyKeySnap = true;
    std::atomic<bool> vocalToMelodyMakeBends = true;
    std::atomic<float> micGain_dB = 0.0f;

    /**
     * @brief Add partials for a specific tone
     * @param toneTotalCents Total cents value of the tone (1200 * octave + cents)
     * @param partials Vector of partials (frequency in Hz, amplitude)
     * @note Currently only one set of partials is supported, so adding new ones clears previous
     */
    void add_partials(int toneTotalCents, partialsVec partials) {
        std::sort(partials.begin(), partials.end(),
                  [](const auto &p1, const auto &p2) { return p1.first < p2.first; });
        std::lock_guard<std::mutex> lock(partialsMutex);
        // ========================================================================================
        // ==================================== IN DEVELOPMENT ====================================
        // ========================================================================================
        // DissonanceMeter CURRENTLY USES ONLY one set of partials, so there is no need to store
        // several
        if (!tonesPartials.empty()) {
            tonesPartials.clear();
        }
        tonesPartials[toneTotalCents] = partials;
        // ========================================================================================
    }

    /**
     * @brief Remove partials for a specific tone
     * @param toneTotalCents Total cents value of the tone to remove
     * @return true if partials were removed, false if tone was not found
     */
    bool remove_partials(int toneTotalCents) {
        std::lock_guard<std::mutex> lock(partialsMutex);
        return tonesPartials.erase(toneTotalCents) > 0;
    }

    /**
     * @brief Set all tones' partials at once
     * @param newTonesPartials Map of tone total cents to partials vectors
     */
    void set_tonesPartials(std::map<int, partialsVec> newTonesPartials) {
        std::lock_guard<std::mutex> lock(partialsMutex);
        tonesPartials = newTonesPartials;
    }

    /**
     * @brief Get all tones' partials
     * @return Map of tone total cents to partials vectors
     */
    std::map<int, partialsVec> get_tonesPartials() {
        std::lock_guard<std::mutex> lock(partialsMutex);
        return tonesPartials;
    }

    /**
     * @brief Set the number of bars in the timeline
     * @param new_num_bars Number of bars (must be >= min_num_bars and <= max_num_bars)
     * @note Also updates the zones border point
     */
    void set_num_bars(int new_num_bars) {
        num_bars = new_num_bars;
        zones.setBorderPoint(float(num_bars));
    }
    int get_num_bars() { return num_bars; }

    // ~~~~~~~~~~~~~~~~~~~~~~~~ Not saved params ~~~~~~~~~~~~~~~~~~~~~~~~
    bool isCamFixedOnPlayHead = false;
    bool timeSnap = true;
    bool keySnap = false;
    bool editRatiosMarks = false;
    bool hideCents = false;
    bool showGhostNotesKeys = true;
    bool generateNewKeys = false;
    int numNewGenKeys = 3;
    int minDistExistNewKeys = 20;
    int minDistBetweenNewKeys = 40;
    GenNewKeysTactics genNewKeysTactics = GenNewKeysTactics::DiverseIntervals;
    std::set<int> ghostNotesChannels = {};
    // ================== Intellectual ==================
    // Partials/dissonance
    std::atomic<bool> findPartialsMode = false;
    int plotPartialsTotalCents = 4 * 1200 + 900;
    int plotDissonanceTotalCents = 4 * 1200 + 900;
    bool plotPartialsInterp = true;
    // Pitch memory
    bool showPitchesMemoryTraces = false;
    bool showKeysHarmonicity = false;
    // ================= Vocal to melody =================
    std::atomic<bool> vocalToMelody = false;

    Parameters() : zones(true, float(num_bars)) {}

  private:
    // ~~~~~~~~~~~~~~~~~~~~~~~~~ Saved params ~~~~~~~~~~~~~~~~~~~~~~~~~
    int num_bars = 6;
    // ================== Intellectual ==================
    /**
     * Partials/dissonance
     * key - total cents of tone, value - partials of tone
     * Partials consist of pairs {freq, amp}, where freq is in Hz and amp is linear (gain)
     */
    std::map<int, partialsVec> tonesPartials = {{5700, {{A4Freq, 1.0f}}}};
    // GUI and processor threads can try to get/set partials at the same time
    std::mutex partialsMutex;
    // ==================================================
};
} // namespace audio_plugin