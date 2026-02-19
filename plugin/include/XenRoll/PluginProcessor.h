#pragma once

#include "AccumulatingBuffer.h"
#include "Note.h"
#include "Parameters.h"
#include "PartialsFinder.h"
#include "PitchDetectorMPM.h"
#include "PluginInstanceManager.h"
#include "RelevanceQueue.h"
#include <algorithm>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

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
    void setManuallyPlayedNotesTotalCents(const std::set<int> newManuallyPlayedNotesTotalCents);

    const std::vector<Note> &getNotes();
    std::vector<Note> getOtherInstancesNotes();

    /**
     * @brief Get current playhead time
     * @return Playhead time in beats
     */
    float getPlayHeadTime();

    /**
     * @brief Get BPM, numerator and denominator
     * @return Tuple of (BPM, numerator, denominator)
     */
    std::tuple<float, int, int> getBpmNumDenom();
    std::set<int> getAllCurrPlayedNotesTotalCents();

    /**
     * @brief Check if there is a pitch overflow (more than 128 unique pitches)
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

    // ====================================== VOCAL TO NOTES ======================================
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

    bool isPlaying() { return wasPlaying; }
    // ============================================================================================

  private:
    // ====================================== INSTANCES SYNC ======================================
    std::unique_ptr<PluginInstanceManager> pluginInstanceManager;
    bool isActive = false;
    // ============================================================================================

    // ===================================== PARTIALS FINDING =====================================
    double freqs12EDO[128]{0.0};
    std::array<bool, 128> activeMidiNotes{false};
    bool wasPianoRoll = true;
    int recordingMidiNote;
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

    // ====================================== VOCAL TO NOTES ======================================
    // rec = recording
    std::mutex recNoteMutex;
    Note recNote;                        ///< Note that is currently being recorded
    std::atomic<bool> isRecNote = false; ///< Recording a note now?
    std::mutex recNotesVecMutex;
    std::vector<Note> recNotesVec; ///< All notes that have already been recorded in this session
    std::atomic<float> currRecVolume = 0.0f; ///< Current detected volume in dB
    std::set<int> recKeys;                   ///< Keys of recorded notes

    // Pitch detector for vocal input
    std::unique_ptr<pitch_detection::PitchDetectorMPM> pitchDetector;

    // Keys from UI. Are needed only for `key snap` mode
    std::mutex keysMutex;
    std::set<int> keys;

    // Vocal recording state
    float currentVocalFreq = 0.0f;   ///< Current detected frequency in Hz
    int recNoteStartTotalCents = -1; ///< Start pitch of current recording note in total cents
    int recNoteMinTotalCents = -1;   ///< Min pitch of current recording note in total cents
    int recNoteMaxTotalCents = -1;   ///< Max pitch of current recording note in total cents
    int currentVocalTotalCents = -1; ///< Current detected pitch in total cents
    float noteStartTime = 0.0f;      ///< Start time of current note being recorded (in beats)
    ///< Time when min pitch was recorded for the note that is currently being recorded (in beats)
    float noteMinPitchTime = 0.0f;
    ///< Time when max pitch was recorded for the note that is currently being recorded (in beats)
    float noteMaxPitchTime = 0.0f;

    // Buffer for accumulating samples for FFT analysis
    static constexpr int vocalFFTSize = 8192;
    std::vector<float> vocalAccumBuffer; ///< Accumulation buffer for vocal input
    int vocalAccumCount = 0;             ///< Number of samples currently in buffer

    void processVocalInput(const juce::AudioBuffer<float> &buffer, int numSamples,
                           double sampleRate, double beatsPerBlock, double playHeadTime,
                           double bpm);
    void updateRecordingNote();
    void fixateRecordingNote(); ///< Before calling make sure that isRecNote == true
    void startRecordingNote();

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

    // INFO: totalCents = octave*1200 + cents

    std::vector<Note> notes;
    ///< Contains midi note number (0-127) for each note from notes vector
    std::vector<int> notesIndexes;
    /**
     * Contains midi note number (0-127) for each note that is played manually (from
     * manuallyPlayedNotesTotalCents)
     */
    std::vector<int> manuallyPlayedNotesIndexes;
    ///< Contains frequency in Hz for each midi note
    double freqs[128];
    ///< if midi note is bending it has != -1 original totalCents here
    int beforeBendTotalCents[128];
    std::atomic<bool> wasPlaying = false;

    /**
     * TotalCents of notes from notes vector (so from piano roll) that are currently played (bends
     * are not taken into account)
     */
    std::set<int> currPlayedNotesTotalCents;
    ///< TotalCents of notes(keys) that are currently played manually (not from piano roll)
    std::set<int> manuallyPlayedNotesTotalCents;
    // Maybe this mutex is unnecessary
    std::mutex manPlNotesTotCentsMutex;
    /**
     * Shows whether the i-th note(key) from manuallyPlayedNotesTotalCents was played manually
     * before (this determines whether it needs noteOn or it just continues to play)
     */
    std::vector<bool> manuallyPlayedNotesAreNew;

    /**
     * Midi note numbers of all notes that are playing now (including notes and
     * manuallyPlayedNotesTotalCents)
     */
    std::set<int> currPlayedNotesIndexes;

    /**
     * A flag that indicates whether we have started playing notes (noteOn) from
     * manuallyPlayedNotesTotalCents
     */
    bool startedPlayingManuallyPressedNotes;

    /**
     * We need to save as much as possible frequencies in freqs[128] for notes that were played
     * manually (and there are no notes with same pitch in piano roll), because if we will change
     * frequency of the note that was just played, then the residual sound (for example from reverb)
     * will also change frequency.
     */
    RelevanceQueue<double> manPlNotesTotCentsHistory;

    /**
     * We have only 128 midi notes (cause freqs[128]) in single midi track so when we try to have
     * more unique notes (pitches) we get overflow state then user needs to remove some notes (from
     * piano roll and manually pressed)
     */
    bool pitchesOverflow = false;
    bool editorKnowsAboutOverflow = false;

    std::atomic<double> playHeadTime = 0.0; ///< in beats

    double getNoteFreq(const Note &note);
    double getFreqFromTotalCents(float totalCents);
    int getTotalCentsFromFreq(double freq);

    /**
     * @brief Find index in freqs array for a given frequency
     * @param freq Frequency to find
     * @return Index (0-127) or -1 if not found
     */
    int findFreqInd(double freq);

    /**
     * @brief Prepare notes for playback (assign MIDI note numbers and calculate frequencies)
     */
    void prepareNotes();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
} // namespace audio_plugin
