#pragma once

#include "XenRoll/common/CircularStack.h"
#include "XenRoll/data/PartialsTypes.h"
#include "XenRoll/data/RatioMark.h"
#include "XenRoll/data/Theme.h"
#include "XenRoll/data/Zones.h"
#include <atomic>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
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

struct State {
    int numBars;
    std::vector<Note> notes;
    std::vector<RatioMark> ratiosMarks;
};

class Parameters {
  public:
    enum GenNewKeysTactics { DiverseIntervals = 1, Random = 2 };
    static const juce::Array<juce::String> getGenNewKeysTacticsNames() {
        return {"+diverse intervals", "random"};
    }

    enum TuningType { MPE = 1, MTS_ESP = 2 };
    static const juce::Array<juce::String> getTuningTypeNames() { return {"MPE", "MTS"}; }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~ HOTKEYS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    enum hotkeys {
        timeSnap_withAlt = 'a',
        keySnap_withAlt = 's',
        editRatiosMarks_withAlt = 'd',
        pitchMemory_withAlt = 'f'
    };

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~ Constants ~~~~~~~~~~~~~~~~~~~~~~~~~~~
    static constexpr int num_octaves = 10;
    static constexpr float init_octave_height_px = 300.0f;
    static constexpr float init_bar_width_px = 400.0f;
    static constexpr float max_bar_width_px = 4000.0f;
    static constexpr float max_octave_height_px = 5000.0f;
    static constexpr float max_note_height_scale = 2.0f;
    static constexpr float min_subdiv_width_px = 4.0f; // for drawing subdiv lines
    static constexpr float min_beat_width_px = 6.0f;   // for drawing beat lines
    static constexpr float min_bar_width_px = 30.0f;   // for drawing bar lines
    // ===== Vocal to melody =====
    static constexpr float minVocalVolume_dB = -60.0f;
    static constexpr float maxVocalVolume_dB = 0.0f;
    // ==========================
    static constexpr float defaultVelocity = 100.0f / 127;
    static constexpr float minNoteDuration = 1.0f / 256;
    // ==== playDraggedNotes ====
    static constexpr int maxCreatedNoteAuditionTime = 1500; ///< in milliseconds
    static constexpr int noKeySnapPlDragNotesDelayTime = 70; ///< in milliseconds
    // ====== OSC Settings ======
    const juce::String oscHost = "127.0.0.1";
    static constexpr int oscPort = 8000;

    // ~~~~~~~~~~~~~~~~~~~~~~~~ Minimum values ~~~~~~~~~~~~~~~~~~~~~~~~
    static constexpr int min_editorWidth = 1430;
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

    // ~~~~~~~~~~~~~~~~~~~~~~~~~ Saved params ~~~~~~~~~~~~~~~~~~~~~~~~~
    int editorWidth = 1430;
    int editorHeight = 700;
    int num_beats = 4;
    int num_subdivs = 4;
    int start_octave = 2;
    float lastDuration = 1.0f;
    float lastVelocity = defaultVelocity;
    juce::Point<float> lastViewPos = {0, 0}; // float for accuracy when zooming (scrolling)
    float octave_height_px = init_octave_height_px;
    float bar_width_px = init_bar_width_px;
    bool showGhostNotesKeys = true;
    std::set<int> ghostNotesChannels = {}; ///< for MTS-ESP
    std::set<int> ghostNotesInstIds = {};  ///< for MPE
    std::atomic<double> A4Freq = 440.0;
    float noteRectHeightCoef = 0.04f;
    bool constNoteRectHeight = false;
    Zones zones;
    std::vector<RatioMark> ratiosMarks = {};
    bool autoCorrectRatiosMarks = true;
    int maxDenRatiosMarks = 40;
    int goodEnoughErrorRatiosMarks = 4;
    Theme::ThemeType themeType = Theme::ThemeType::Gray;
    Theme theme;
    ///< For MTS-ESP. -1 means uninited state, range: [0,...,15]
    int channelIndex = -1;
    ///< For MPE. -1 means uninited state, range: [0, 1, 2, ...]
    int instanceId = -1;
    float maxChordDtimeClockDiagram = 1.0f / 32;       ///< in bars
    std::atomic<bool> resetPitchBendOnNoteOff = false; ///< setting for MPE tuning
    ///< Possible values: {12, 24, 48, 96}. Setting for MPE tuning
    std::atomic<int> semiBendRangeMPE = 48;
    bool channelsEconomyModeMPE = false; ///< Setting for MPE tuning
    // ================== Intellectual ==================
    // Partials/dissonance
    std::atomic<int> findPartialsFFTSize = 8192;
    std::atomic<float> findPartialsdBThreshold = -60.0f;
    std::atomic<PartialsFindPosStrat> findPartialsStrat = PartialsFindPosStrat::maxSpectralFlatness;
    float roughCompactFrac = 0.0f;
    float dissonancePow = 1.1f;
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
        zones.setBorderPoint(static_cast<float>(num_bars));
    }
    int get_num_bars() { return num_bars; }

    // ~~~~~~~~~~~~~~~~~~~~~~~~ Not saved params ~~~~~~~~~~~~~~~~~~~~~~~~
    bool isCamFixedOnPlayHead = false;
    bool timeSnap = true;
    bool keySnap = false;
    bool editRatiosMarks = false;
    bool hideCents = false;
    bool generateNewKeys = false;
    int numNewGenKeys = 3;
    int minDistExistNewKeys = 20;
    int minDistBetweenNewKeys = 40;
    GenNewKeysTactics genNewKeysTactics = GenNewKeysTactics::DiverseIntervals;
    bool recordManuallyPlayedNotes = false;
    bool showClockDiagram = false;
    bool showDebugOverlay = false;
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
    // ===================================================
    CircularStack<State> stateHistory{100};

    ///< For normal use (get read-only local tuningType)
    TuningType getTuningType() const { return tuningType; }

    ///< Sets global tuning type (not affecting local tuningType)
    static void setTuningType(TuningType newType) { globalTuningType.store(newType); }

    ///< Get global tuningType. For getStateInformation in PluginProcessor ONLY!
    static TuningType getGlobalTuningType() { return globalTuningType.load(); }

    ///< Syncs local tuningType with global. For setStateInformation in PluginProcessor ONLY!
    void applyGlobalTuningType() { tuningType = globalTuningType.load(); }

    Parameters() : zones(true, static_cast<float>(num_bars)), theme(themeType) {
        tuningType = globalTuningType;
    }

  private:
    // ~~~~~~~~~~~~~~~~~~~~~~~ Not saved params ~~~~~~~~~~~~~~~~~~~~~~~
    TuningType tuningType;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~ Saved params ~~~~~~~~~~~~~~~~~~~~~~~~~
    ///< Global tuning type that is same between instances
    inline static std::atomic<TuningType> globalTuningType = TuningType::MPE;
    int num_bars = 6;
    // ================== Intellectual ==================
    /**
     * Partials/dissonance
     * key - total cents of tone, value - partials of tone
     * Partials consist of pairs {freq, amp}, where freq is in Hz and amp is linear (gain)
     */
    std::map<int, partialsVec> tonesPartials = {
        {5700, {{static_cast<float>(A4Freq.load()), 1.0f}}}};
    // GUI and processor threads can try to get/set partials at the same time
    std::mutex partialsMutex;
    // ==================================================

    juce::File getProjectDirectory() {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("XenRoll");
    }
};
} // namespace audio_plugin