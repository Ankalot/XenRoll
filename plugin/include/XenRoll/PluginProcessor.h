#pragma once

#include "AccumulatingBuffer.h"
#include "Note.h"
#include "Parameters.h"
#include "PartialsFinder.h"
#include "PluginInstanceManager.h"
#include "RelevanceQueue.h"
#include <algorithm>
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
    void setManuallyPlayedNotesTotalCents(const std::set<int> newManuallyPlayedNotesTotalCents);

    const std::vector<Note> &getNotes();
    std::vector<Note> getOtherInstancesNotes();
    float getPlayHeadTime();
    std::tuple<float, int, int> getBpmNumDenom();
    std::set<int> getAllCurrPlayedNotesTotalCents();

    bool thereIsPitchOverflow();

    // Parameters are stored here
    Parameters params;

    bool getIsActive() { return isActive; }

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
    void tryStartRecording();
    // ============================================================================================

    // INFO: totalCents = octave*1200 + cents

    std::vector<Note> notes;
    // Contains midi note number (0-127) for each note from notes vector
    std::vector<int> notesIndexes;
    // Contains midi note number (0-127) for each note that is played manually
    //     (from manuallyPlayedNotesTotalCents)
    std::vector<int> manuallyPlayedNotesIndexes;
    // Contains frequency in Hz for each midi note
    double freqs[128];
    // if midi note is bending it has != -1 original totalCents here
    int beforeBendTotalCents[128];
    bool wasPlaying = false;

    // TotalCents of notes from notes vector (so from piano roll) that are currently played
    // (bends are not taken into account)
    std::set<int> currPlayedNotesTotalCents;
    // TotalCents of notes(keys) that are currently played manually (not from piano roll)
    std::set<int> manuallyPlayedNotesTotalCents;
    // Maybe this mutex is unnecessary
    std::mutex manPlNotesTotCentsMutex;
    // Shows whether the i-th note(key) from manuallyPlayedNotesTotalCents was played manually
    // before (this determines whether it needs noteOn or it just continues to play)
    std::vector<bool> manuallyPlayedNotesAreNew;

    // Midi note numbers of all notes that are playing now
    // (including notes and manuallyPlayedNotesTotalCents)
    std::set<int> currPlayedNotesIndexes;

    // A flag that indicates whether we have started playing notes (noteOn) from
    // manuallyPlayedNotesTotalCents
    bool startedPlayingManuallyPressedNotes;

    // We need to save as much as possible frequencies in freqs[128] for notes that
    // were played manually (and there are no notes with same pitch in piano roll),
    // because if we will change frequency of the note that was just played, then
    // the residual sound (for example from reverb) will also change frequency.
    RelevanceQueue<double> manPlNotesTotCentsHistory;

    // We have only 128 midi notes (cause freqs[128]) in single midi track
    // so when we try to have more unique notes (pitches) we get overflow state
    // then user needs to remove some notes (from piano roll and manually pressed)
    bool pitchesOverflow = false;
    bool editorKnowsAboutOverflow = false;

    std::atomic<double> playHeadTime = 0.0;

    double getNoteFreq(const Note &note);
    double getFreqFromTotalCents(float totalCents);
    int getTotalCentsFromFreq(double freq);
    int findFreqInd(double freq);
    void prepareNotes();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
} // namespace audio_plugin
