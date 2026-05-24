#pragma once

#include "XenRoll/common/RelevanceQueue.h"
#include "XenRoll/data/GlobalSettings.h"
#include "XenRoll/data/Note.h"
#include "XenRoll/data/Parameters.h"
#include "XenRoll/processor/audio/AccumulatingBuffer.h"
#include "XenRoll/processor/audio/dsp/PartialsFinder.h"
#include "XenRoll/processor/audio/dsp/PitchDetectorMPM.h"
#include "XenRoll/processor/managers/ChannelsManagerMPE.h"
#include "XenRoll/processor/managers/NotesSharingMPE.h"
#include "XenRoll/processor/managers/PluginInstanceManager.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessor : public juce::AudioProcessor {
  public:
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    void rePrepareNotes();
    void updateNotes(const std::vector<Note> &new_notes);
    /**
     * @brief Set the Manually Played Notes ( = Manually Played Keys in Plugin Editor)
     * @param newManuallyPlayedNotes {totalCents -> velocity}
     */
    void setManuallyPlayedNotes(const std::map<int, float> newManuallyPlayedNotes);

    const std::vector<Note> &getNotes();
    std::vector<Note> getOtherInstancesNotes();

    /**
     * @brief Get current playhead time
     * @return Playhead time in bars
     */
    float getPlayHeadTime();

    /**
     * @brief Get BPM, numerator and denominator
     * @return Tuple of (BPM, numerator, denominator)
     */
    std::tuple<float, int, int> getBpmNumDenom();

    /**
     * @brief Check if there is a pitch overflow
     * @return true if overflow detected
     */
    bool thereIsPitchOverflow();

    // Parameters are stored here
    Parameters params;

    /**
     * @brief Check if plugin instance manager is active
     * @return true if active
     */
    bool getIsActive() { return isActive; }

    bool isPlaying() { return wasPlaying; }

    // ====================================== VOCAL TO MELODY =====================================
    void updateKeys(const std::set<int> &newKeys) {
        std::scoped_lock lock(keysMutex);
        keys = newKeys;
    }

    std::vector<Note> getRecordedNotesFromVocal() {
        std::scoped_lock lock(recNotesVecMutex);
        return recNotesVec;
    }

    int getNumOfRecordedNotesFromVocal() {
        std::scoped_lock lock(recNotesVecMutex);
        return recNotesVec.size();
    }

    bool getIsRecNote() { return isRecNote; }
    Note getRecNote() {
        std::scoped_lock lock(recNoteMutex);
        return recNote;
    }

    void startRecordingVocal();
    void stopRecordingVocal();

    float getCurrRecVolume() { return currRecVolume; }

    /**
     * @brief Update editor's pitch curve to match current pitch curve in processor
     * @param pitchCurveFromEditor Pitch curve from editor
     * @return true if edited it and false otherwise
     */
    bool updatePitchCurveForEditor(PitchCurve &pitchCurveFromEditor) {
        const int pcfeSize = pitchCurveFromEditor.first.size();
        std::scoped_lock lock(pitchCurveMutex);
        const int pcSize = pitchCurve.first.size();
        const int dSize = pcSize - pcfeSize;
        if (dSize > 0) {
            // Add new elements
            pitchCurveFromEditor.first.insert(pitchCurveFromEditor.first.end(),
                                              pitchCurve.first.end() - dSize,
                                              pitchCurve.first.end());
            pitchCurveFromEditor.second.insert(pitchCurveFromEditor.second.end(),
                                               pitchCurve.second.end() - dSize,
                                               pitchCurve.second.end());
            return true;
        } else if (dSize < 0) {
            // This shouldn't happen in theory.
            pitchCurveFromEditor.first = pitchCurve.first;
            pitchCurveFromEditor.second = pitchCurve.second;
            return true;
        }
        // if (pcSize == pcfeSize) they are equal, nothing to do
        return false;
    }
    // ============================================================================================

    std::set<int> getAllInstancesIndexes() {
        if (!isActive) {
            return {};
        }
        if (params.getTuningType() == Parameters::TuningType::MPE) {
            return notesSharingMPE->getAllInstanceIds();
        } else if (params.getTuningType() == Parameters::TuningType::MTS_ESP) {
            return pluginInstanceManager->getAllInstanceChannels();
        }
        return {};
    }

    void changedChannelsEconomyModeMPE() {
        channelsManagerMPE->setEconomyMode(params.channelsEconomyModeMPE);
    }

    void startAuditioning(double newAuditionTime) {
        auditionTime = newAuditionTime;
        stopAuditioning = false;
        auditionChanged = true;
        isAuditioning = true;
    }

    void endAuditioning() {
        isAuditioning = false;
        stopAuditioning = true;
    }

    void setAuditionTime(double newAuditionTime) {
        auditionTime = newAuditionTime;
        auditionChanged = true;
    }

  private:
    // The magic number for defining the new ValueTree format
    static constexpr int MAGIC_NUMBER = 7777777;

    void legacySetStateInformation(const void *data, int sizeInBytes);

    // ====================================== INSTANCES SYNC ======================================
    std::unique_ptr<PluginInstanceManager> pluginInstanceManager; ///< For MTS-ESP
    bool isActive = false;
    std::unique_ptr<NotesSharingMPE> notesSharingMPE; ///< For MPE

    void changeInstanceSync(Parameters::TuningType newTuningType);
    // ============================================================================================

    // ===================================== PARTIALS FINDING =====================================
    double freqs12EDO[128]{0.0};
    std::array<bool, 128> activeMidiNotes{false};
    bool wasPianoRoll = true;
    int recordingMidiNote = -1;
    bool isRecording = false;
    std::unique_ptr<AccumulatingBuffer> partialsFinderBuffer;
    std::shared_ptr<PartialsFinder> partialsFinder;
    std::unique_ptr<juce::ThreadPool> threadPool;

    void startPartialsFinding();

    /**
     * @brief Try to start recording for partials finding
     */
    void tryStartRecording();
    // ============================================================================================

    // ====================================== VOCAL TO MELODY =====================================
    // rec = recording
    std::mutex recNoteMutex;
    Note recNote;                        ///< Note that is currently being recorded
    std::atomic<bool> isRecNote = false; ///< Recording a note now?
    std::mutex recNotesVecMutex;
    std::vector<Note> recNotesVec; ///< All notes that have already been recorded in this session
    std::atomic<float> currRecVolume = 0.0f; ///< Current detected volume in dB
    std::set<int> recKeys;                   ///< Keys of recorded notes (needed for snapping)
    std::mutex pitchCurveMutex;
    PitchCurve pitchCurve;

    // Pitch detector for vocal input
    std::unique_ptr<PitchDetectorMPM> pitchDetector;

    // Keys from UI. Are needed only for `key snap` mode
    std::mutex keysMutex;
    std::set<int> keys;

    // Vocal recording state
    int recNoteStartTotalCents = -1; ///< Start pitch of current recording note in total cents
    int recNoteMinTotalCents = -1;   ///< Min pitch of current recording note in total cents
    int recNoteMaxTotalCents = -1;   ///< Max pitch of current recording note in total cents
    int currentVocalTotalCents = -1; ///< Current detected pitch in total cents
    float noteStartTime = 0.0f;      ///< Start time of current note being recorded (in bars)
    ///< Time when min pitch was recorded for the note that is currently being recorded (in bars)
    float noteMinPitchTime = 0.0f;
    ///< Time when max pitch was recorded for the note that is currently being recorded (in bars)
    float noteMaxPitchTime = 0.0f;

    // Buffer for accumulating samples for FFT analysis
    static constexpr int vocalFFTSize = 4096;
    std::vector<float> vocalAccumBuffer; ///< Accumulation buffer for vocal input
    int vocalAccumCount = 0;             ///< Number of samples currently in buffer
    ///< It is playHeadTime but with delay that is caused by vocalAccumBuffer, in bars
    double pitchTime = 0.0;

    void processVocalInput(const juce::AudioBuffer<float> &buffer, int numSamples,
                           double sampleRate);
    void updateRecordingNote();
    void fixateRecordingNote(); ///< Before calling make sure that isRecNote == true
    void startRecordingNote();
    void vocalIsSilent();

    int freqToTotalCents(float freq);

    /**
     * @brief Find nearest key within maximum cents change
     * @param key Cents of the key
     * @param maxCentsChange Maximum cents change allowed
     * @param keys Set of available keys (in cents)
     * @return Optional total cents of nearest key, or nullopt if none found
     */
    std::optional<int> findNearestKeyWithLimit(int key, int maxCentsChange,
                                               const std::set<int> &keys);

    /**
     * @brief Try to snap note to a key from keys set
     * @param note Note
     * @param keys Set of available keys (in cents)
     * @return true if snapped
     */
    bool trySnapNote(Note &note, const std::set<int> &keys);
    // ============================================================================================

    // ========================================== GENERAL =========================================
    // INFO: totalCents = octave*1200 + cents

    /**
     *              If using MPE tuning:
     * Maximum 15 pitches can be played simultaneously (they occupy channels 2-16)
     *              If using MTS-ESP tuning:
     * We have only 128 midi notes (cause freqs[128]) in single midi track, so max 128 pitches
     *              General:
     * In overflow state user needs to remove some notes (from piano roll and/or manually pressed)
     */
    bool pitchesOverflow = false;
    bool editorKnowsAboutOverflow = false;

    /**
     * If using MTS-ESP tuning:
     *      Prepare notes for playback (assign MIDI note numbers and calculate frequencies)
     * If using MPE tuning:
     *      Just set editorKnowsAboutOverflow and pitchesOverflow to false
     */
    void prepareNotes();

    std::vector<Note> notes;

    std::atomic<bool> wasPlaying = false;

    ///< {totalCents -> velocity} of notes(keys) that are currently played manually
    std::map<int, float> manuallyPlayedNotes;
    std::mutex manPlNotesMutex;

    std::atomic<double> playHeadTime = 0.0; ///< in bars

    std::atomic<bool> isAuditioning = false;
    std::atomic<bool> stopAuditioning = false;
    std::atomic<bool> auditionChanged = false;
    std::atomic<double> auditionTime = 0.0;
    // ============================================================================================

    // ======================================= USING MTS-ESP ======================================
    ///< Contains midi note number (0-127) for each note from notes vector
    std::vector<int> notesIndexes;
    ///< Manually played note's totalCents -> midi note number (0-127)
    std::map<int, int> manPlNoteToMidiNoteMTS;
    ///< Audition note's {index, totalCents} -> midi note number (0-127)
    std::map<std::pair<int, int>, int> auditionNoteToMidiNoteMTS;
    /**
     * Is used to indicate that this frequency is not being used. Freq in Hz.
     * And if it is still used for a veeery short period of time (by mistake?), then there will be
     * no sound. 0.0 is a bad idea, as this may cause some synth plugins to crash (tested on Surge
     * XT).
     */
    const double noFreq = 1e-2;
    ///< Contains frequency in Hz for each midi note
    double freqs[128]{noFreq};
    ///< if midi note is bending it has != -1 original totalCents here
    int beforeBendTotalCents[128];

    /**
     * TotalCents of notes from notes vector (so from piano roll) that are currently played (bends
     * are not taken into account)
     */
    std::set<int> currPlayedNotesTotalCents;

    /**
     * Midi note numbers of all notes that are playing now (including notes and
     * manuallyPlayedNotes)
     */
    std::set<int> currPlayedNotesIndexes;

    /**
     * We need to save as much as possible frequencies in freqs[128] for notes that were played
     * manually (and there are no notes with same pitch in piano roll), because if we will change
     * frequency of the note that was just played, then the residual sound (for example from reverb)
     * will also change frequency.
     */
    RelevanceQueue<double> manPlNotesTotCentsHistory;

    double getNoteFreq(const Note &note);
    double getNoteFreqAtTime(const Note &note, double time);
    double getFreqFromTotalCents(float totalCents);
    int getTotalCentsFromFreq(double freq);

    /**
     * @brief Find index in freqs array for a given frequency
     * @param freq Frequency to find
     * @return Index (0-127) or -1 if not found
     */
    int findFreqInd(double freq);
    // ============================================================================================

    // ========================================= USING MPE ========================================
    // First midi channel is for globale messages, channels 2-16 are for notes
    // In contrast to MTS-ESP, there 2+ notes with same totalCents can be played simultaneously
    //      (useful if these notes have different bend)

    /**
     * No pitch bend = 8192; 0 and 16383 are -semiBendRangeMPE and +semiBendRangeMPE semitones
     * respectively. Is updated in processBlock().
     */
    double centsPerBendMPE = 48 * 100.0 / 8192;

    ///< Is used to correct totalCents taking into account A4 freq. Is updated in processBlock().
    double corrTotalCentsMPE = 0;

    ///< {note's index, note's totalCents} -> {midi channel (2-16), midi note number}
    std::map<std::pair<int, int>, std::pair<int, int>> noteToChAndMidiNoteMPE,
        auditionNoteToChAndMidiNoteMPE;
    ///< Manually played note's totalCents -> {midi channel (2-16), midi note number}
    std::map<int, std::pair<int, int>> manPlNoteToChAndMidiNoteMPE;
    std::unique_ptr<ChannelsManagerMPE> channelsManagerMPE;

    /**
     * totalcents of notes that are currently played from piano roll -> number of that notes
     * We need map instead of set because there can be several notes that are been played with
     * same totalCents (but with different note bend, for example)
     */
    std::map<int, int> currPlayedNotesTotalCentsMPE;

    void addCurrPlayedNotesTotalCentsMPE(int totalCents) {
        if (currPlayedNotesTotalCentsMPE.contains(totalCents)) {
            currPlayedNotesTotalCentsMPE[totalCents]++;
        } else {
            currPlayedNotesTotalCentsMPE[totalCents] = 1;
        }
    }

    void delCurrPlayedNotesTotalCentsMPE(int totalCents) {
        if (currPlayedNotesTotalCentsMPE.contains(totalCents)) {
            currPlayedNotesTotalCentsMPE[totalCents]--;
            if (currPlayedNotesTotalCentsMPE[totalCents] == 0) {
                currPlayedNotesTotalCentsMPE.erase(totalCents);
            }
        }
    }

    /**
     * @brief Calculate midi note and midi pitch bend from totalCents
     * @param totalCents octave*1200 + cents
     * @return std::pair<int, int>: first value is midi note in range [0...127] and
     * second value is MPE bend in range [0...16383], where 8192 is no bend
     */
    std::pair<int, int> calcMidiNoteAndBendMPE(int totalCents) {
        double correctedTotalCents = totalCents + corrTotalCentsMPE;
        // 0 totalCents = C0 = 12th midi note
        int midiNote = juce::jlimit(0, 127, 12 + juce::roundToInt(correctedTotalCents / 100.0));
        double bendCents = correctedTotalCents - (midiNote - 12) * 100;
        int bendMPE = juce::jlimit(0, 16383, 8192 + juce::roundToInt(bendCents / centsPerBendMPE));
        return std::make_pair(midiNote, bendMPE);
    }

    ///< Calculate midi bend based on Note.bend while playing it
    int calcBendMPE(const Note &note, double currTime) {
        double totalCents = note.octave * 1200 + note.cents + corrTotalCentsMPE;
        double bendCentsBase = totalCents - juce::roundToInt(totalCents / 100.0) * 100;
        double bendCents = bendCentsBase + note.bend * (currTime - note.time) / note.duration;
        int bendMPE = juce::jlimit(0, 16383, 8192 + juce::roundToInt(bendCents / centsPerBendMPE));
        return bendMPE;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
} // namespace audio_plugin
