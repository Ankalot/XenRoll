#include "XenRoll/processor/PluginProcessor.h"
#include "XenRoll/common/Helpers.h"
#include "XenRoll/editor/PluginEditor.h"
#include "XenRoll/processor/audio/dsp/PitchDetectorMPM.h"
#include <algorithm>

namespace audio_plugin {
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         ),
      manPlNotesTotCentsHistory(128), params() {

    // INSTANCES SYNC (params.getTuningType() IS DEFAULT HERE, BECAUSE IT ONLY WILL
    //                 BE READ IN setStateInformation)
    if (params.getTuningType() == Parameters::TuningType::MTS_ESP) {
        pluginInstanceManager = std::make_unique<PluginInstanceManager>();
        isActive = pluginInstanceManager->getIsActive();
        params.channelIndex = pluginInstanceManager->getChannelIndex();
    } else if (params.getTuningType() == Parameters::TuningType::MPE) {
        notesSharingMPE = std::make_unique<NotesSharingMPE>(params.instanceId);
        isActive = notesSharingMPE->getIsActive();
        params.instanceId = notesSharingMPE->getInstanceId();
    }

    // For MPE use only:
    channelsManagerMPE = std::make_unique<ChannelsManagerMPE>(params.channelsEconomyModeMPE);

    // PARTIALS FINDING
    threadPool = std::make_unique<juce::ThreadPool>(1);
    partialsFinder = std::make_shared<PartialsFinder>();
    wasPianoRoll = !params.findPartialsMode.load();
    for (int i = 0; i < 128; ++i) {
        freqs12EDO[i] = getFreqFromTotalCents(i * 100.0f);
    }
    partialsFinderBuffer = std::make_unique<AccumulatingBuffer>();

    // VOCAL TO MELODY
    pitchDetector = std::make_unique<PitchDetectorMPM>(vocalFFTSize);
    vocalAccumBuffer.resize(vocalFFTSize);
    vocalAccumCount = 0;

    std::fill(std::begin(beforeBendTotalCents), std::end(beforeBendTotalCents), -1);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

void AudioPluginAudioProcessor::changeInstanceSync(Parameters::TuningType newTuningType) {
    // 1. Make processBlock not execute
    std::scoped_lock lock(changeInstanceSyncMutex);

    // 2. Prepare new manager
    if (newTuningType == Parameters::TuningType::MTS_ESP) {
        pluginInstanceManager = std::make_unique<PluginInstanceManager>();
        isActive = pluginInstanceManager->getIsActive();
        params.channelIndex = pluginInstanceManager->getChannelIndex();
    } else if (newTuningType == Parameters::TuningType::MPE) {
        notesSharingMPE = std::make_unique<NotesSharingMPE>(params.instanceId);
        isActive = notesSharingMPE->getIsActive();
        params.instanceId = notesSharingMPE->getInstanceId();
    }

    // 3. Managers are ready, so now can set new tuning type
    params.setTuningType(newTuningType);
    params.applyGlobalTuningType();

    // 4. Clean up the manager that's no longer needed (opposite of newTuningType)
    if (newTuningType == Parameters::TuningType::MTS_ESP) {
        notesSharingMPE.reset();
        params.instanceId = -1;
    } else if (newTuningType == Parameters::TuningType::MPE) {
        pluginInstanceManager.reset();
        params.channelIndex = -1;
    }
}

const juce::String AudioPluginAudioProcessor::getName() const {
    return "XenRoll"; // JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int AudioPluginAudioProcessor::getNumPrograms() {
    return 1; // NB: some hosts don't cope very well if you tell them there are 0
              // programs, so this should be at least 1, even if you're not
              // really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram() { return 0; }

void AudioPluginAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }

const juce::String AudioPluginAudioProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String &newName) {
    juce::ignoreUnused(index, newName);
}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused(samplesPerBlock);
}

void AudioPluginAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

int AudioPluginAudioProcessor::findFreqInd(double freq) {
    for (int i = 0; i < 128; ++i) {
        if (std::abs(freqs[i] - freq) < 1e-6) {
            return i;
        }
    }
    return -1;
}

void AudioPluginAudioProcessor::startPartialsFinding() {
    threadPool->addJob([buf = partialsFinderBuffer->extractAndClear(), rmn = recordingMidiNote,
                        pf = partialsFinder, pars = &params,
                        strat = params.findPartialsStrat.load(),
                        fftSize = params.findPartialsFFTSize.load(),
                        dbThr = params.findPartialsdBThreshold.load(), sr = getSampleRate()] {
        if (sr != 0) {
            pf->setSampleRate_(sr);
        }
        pf->setFFTSize(fftSize);
        pf->setPosFindStrat(strat);
        pf->setdBThr(dbThr);
        auto partials = pf->findPartials(buf);
        // DON'T ADD EMPTY PARTIALS, THIS IS BAD RESULT!
        if (!partials.empty()) {
            pars->add_partials(rmn * 100, partials);
        }
    });
    isRecording = false;
    recordingMidiNote = -1;
}

// there should be only single `true` in activeMidiNotes array
void AudioPluginAudioProcessor::tryStartRecording() {
    auto it = std::find(activeMidiNotes.begin(), activeMidiNotes.end(), true);
    recordingMidiNote = std::distance(activeMidiNotes.begin(), it);
    if (recordingMidiNote < params.num_octaves * 12) {
        isRecording = true;
    }
}

// ====================================== VOCAL TO MELODY ======================================

bool AudioPluginAudioProcessor::trySnapNote(Note &note, const std::set<int> &keys) {
    if (keys.empty())
        return false;

    int totalCents = note.octave * 1200 + note.cents;
    int newTotalCents = findNearestKeyTotalCents(totalCents, keys, params.num_octaves);
    int maxCentsChange = params.vocalToMelodyDCents;
    int diff = std::abs(totalCents - newTotalCents);
    if (diff <= maxCentsChange) {
        note.octave = newTotalCents / 1200;
        note.cents = newTotalCents % 1200;
        return true;
    }
    return false;
}

void AudioPluginAudioProcessor::fixateRecordingNote() {
    Note currentNote;
    {
        std::scoped_lock lock(recNoteMutex);
        currentNote = recNote;
    }
    float finalDuration = pitchTime - currentNote.time;
    if (finalDuration > 0) {
        currentNote.duration = finalDuration;
    }

    if (currentNote.duration >= params.vocalToMelodyMinNoteDuration) {
        if (params.vocalToMelodyKeySnap) {
            if (!trySnapNote(currentNote, keys)) {
                trySnapNote(currentNote, recKeys);
            }
        } else {
            trySnapNote(currentNote, recKeys);
        }

        std::scoped_lock lock(recNotesVecMutex);
        recNotesVec.push_back(currentNote);
        recKeys.insert(currentNote.cents);
    }

    isRecNote = false;
}

void AudioPluginAudioProcessor::startRecordingNote() {
    Note newNote;
    newNote.octave = currentVocalTotalCents / 1200;
    newNote.cents = currentVocalTotalCents % 1200;
    newNote.time = pitchTime;
    newNote.duration = 0.0f;
    newNote.velocity = params.defaultVelocity;
    newNote.isSelected = true;
    newNote.bend = 0;

    recNoteStartTotalCents = currentVocalTotalCents;
    recNoteMinTotalCents = recNoteStartTotalCents;
    recNoteMaxTotalCents = recNoteStartTotalCents;
    {
        std::scoped_lock lock(recNoteMutex);
        recNote = newNote;
    }
    isRecNote = true;
    noteStartTime = pitchTime;
    noteMinPitchTime = noteStartTime;
    noteMaxPitchTime = noteStartTime;
}

int AudioPluginAudioProcessor::freqToTotalCents(float freq) {
    if (freq <= 0.0f || params.A4Freq.load() <= 0.0f) {
        return -1;
    }
    // A4 = 440 Hz is at octave 4, 900 cents (MIDI note 69)
    // totalCents = 4 * 1200 + 900 = 5700
    const int A4TotalCents = 4 * 1200 + 900;
    double cents = 1200.0 * std::log2(freq / params.A4Freq.load());
    return juce::roundToInt(A4TotalCents + cents);
}

void AudioPluginAudioProcessor::vocalIsSilent() {
    currentVocalTotalCents = -1;
    if (isRecNote) {
        fixateRecordingNote();
    }
    // Even if params.vocalToMelodyGenCurve is false (so everything will work properly
    //    even if we turn this mode on and off while recording)
    const int pitchCurveSize = pitchCurve.first.size();
    if ((pitchCurveSize > 0) && (pitchCurve.second[pitchCurveSize - 1] != -1)) {
        std::scoped_lock lock(pitchCurveMutex);
        pitchCurve.first.push_back(pitchTime);
        pitchCurve.second.push_back(-1);
    }
}

void AudioPluginAudioProcessor::processVocalInput(const juce::AudioBuffer<float> &buffer,
                                                  int numSamples, double sampleRate) {
    if (!pitchDetector) {
        return;
    }

    // Apply mic gain
    float gainLinear = GlobalSettings::getInstance().getMicGainLinear();
    const float *channelData = buffer.getReadPointer(0);

    // Calculate current volume for visualization
    float sumSquares = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        float sample = channelData[i] * gainLinear;
        sumSquares += sample * sample;
    }
    float rms = std::sqrt(sumSquares / numSamples);
    float volume_dB = juce::Decibels::gainToDecibels(rms + 1e-10f);
    currRecVolume = juce::jlimit(params.minVocalVolume_dB, params.maxVocalVolume_dB, volume_dB);

    // Check if signal is too weak to be valid
    if (volume_dB <= params.minVocalVolume_dB) {
        vocalIsSilent();
        vocalAccumCount = 0;
        return;
    }

    int samplesProcessed = 0;
    int inputOffset = 0;

    while (samplesProcessed < numSamples) {
        int spaceAvailable = vocalFFTSize - vocalAccumCount;
        int samplesToCopy = std::min(numSamples - samplesProcessed, spaceAvailable);

        for (int i = 0; i < samplesToCopy; ++i) {
            vocalAccumBuffer[vocalAccumCount + i] = channelData[inputOffset + i] * gainLinear;
        }

        vocalAccumCount += samplesToCopy;
        samplesProcessed += samplesToCopy;
        inputOffset += samplesToCopy;

        // When we have enough samples for FFT, perform analysis
        if (vocalAccumCount >= vocalFFTSize) {
            // Analyze pitch from the accumulated buffer using MPM
            float detectedFreq = pitchDetector->detectPitch(vocalAccumBuffer, sampleRate,
                                                            currentVocalTotalCents == -1);

            if (detectedFreq > 0.0f) {
                int newTotalCents = freqToTotalCents(detectedFreq);
                if (newTotalCents >= 0) {
                    currentVocalTotalCents = newTotalCents;
                    if (params.vocalToMelodyGenNotes) {
                        updateRecordingNote();
                    } else if (isRecNote) {
                        fixateRecordingNote();
                    }
                    if (params.vocalToMelodyGenCurve && (pitchTime >= 0)) {
                        std::scoped_lock lock(pitchCurveMutex);
                        pitchCurve.first.push_back(pitchTime);
                        pitchCurve.second.push_back(currentVocalTotalCents);
                    }
                } else {
                    vocalIsSilent();
                }
            } else {
                vocalIsSilent();
            }

            // Shifting the buffer with overlap (hop size = numSamples for uniformity)
            int hopSize = numSamples;
            int samplesToKeep = vocalFFTSize - hopSize;

            if (samplesToKeep > 0) {
                // We leave the overlapping samples for the next analysis.
                std::copy(vocalAccumBuffer.begin() + hopSize,
                          vocalAccumBuffer.begin() + vocalFFTSize, vocalAccumBuffer.begin());
                vocalAccumCount = samplesToKeep;
            } else {
                vocalAccumCount = 0;
            }
        }
    }
}

void AudioPluginAudioProcessor::updateRecordingNote() {
    if (currentVocalTotalCents < 0) {
        return;
    }

    const int dCentsThreshold = params.vocalToMelodyDCents;

    if (!isRecNote.load()) {
        if (pitchTime >= 0) {
            startRecordingNote();
        }
    } else {
        // Check if pitch changed enough to start a new note
        int curr_start_pitchDiff = std::abs(currentVocalTotalCents - recNoteStartTotalCents);
        bool startNewNote = false;
        if (params.vocalToMelodyMakeBends) {
            float currTime = pitchTime;
            float slope =
                (currentVocalTotalCents - recNoteStartTotalCents) / (currTime - noteStartTime);

            float bendedAtMinTotalCentsTime =
                recNoteStartTotalCents + (noteMinPitchTime - noteStartTime) * slope;

            float bendedAtMaxTotalCentsTime =
                recNoteStartTotalCents + (noteMaxPitchTime - noteStartTime) * slope;

            startNewNote =
                (std::abs(bendedAtMinTotalCentsTime - recNoteMinTotalCents) > dCentsThreshold) ||
                (std::abs(bendedAtMaxTotalCentsTime - recNoteMaxTotalCents) > dCentsThreshold);
        } else {
            startNewNote = curr_start_pitchDiff > dCentsThreshold;
        }

        Note currentNote;
        {
            std::scoped_lock lock(recNoteMutex);
            currentNote = recNote;
        }

        float currentDuration = pitchTime - currentNote.time;
        if (currentDuration < 0) {
            startNewNote = true;
            std::scoped_lock lock(pitchCurveMutex);
            pitchCurve.first.push_back(recNote.time + recNote.duration + 1e-4);
            pitchCurve.second.push_back(-1);
        }

        if (startNewNote) {
            // End current note and start a new one
            fixateRecordingNote();
            if (pitchTime >= 0) {
                startRecordingNote();
            }
        } else {
            // Tweak pitch (base and/or bend)
            if (!params.vocalToMelodyMakeBends || (curr_start_pitchDiff < dCentsThreshold / 2)) {
                // Alter base pitch (if (no bends mode) OR (almost no bend))
                const int alteredVocalTotalCents =
                    ((currentNote.octave * 1200 + currentNote.cents) + currentVocalTotalCents) / 2;
                currentNote.octave = alteredVocalTotalCents / 1200;
                currentNote.cents = alteredVocalTotalCents % 1200;
            } else {
                // Make bend
                currentNote.bend =
                    currentVocalTotalCents - (currentNote.octave * 1200 + currentNote.cents);
            }
            currentNote.duration = currentDuration;

            if (currentVocalTotalCents > recNoteMaxTotalCents) {
                noteMaxPitchTime = pitchTime;
                recNoteMaxTotalCents = currentVocalTotalCents;
            } else if (currentVocalTotalCents < recNoteMinTotalCents) {
                noteMinPitchTime = pitchTime;
                recNoteMinTotalCents = currentVocalTotalCents;
            }

            std::scoped_lock lock(recNoteMutex);
            recNote = currentNote;
        }
    }
}

void AudioPluginAudioProcessor::startRecordingVocal() {
    // Clear previously recorded notes
    {
        std::scoped_lock lock(recNotesVecMutex);
        recNotesVec.clear();
    }

    // Clear recorded keys
    recKeys.clear();

    // Clear pitch curve
    {
        std::scoped_lock lock(pitchCurveMutex);
        pitchCurve.first.clear();
        pitchCurve.second.clear();
    }

    // Reset recording state
    isRecNote = false;
    recNoteStartTotalCents = -1;
    recNoteMinTotalCents = -1;
    recNoteMaxTotalCents = -1;
    currentVocalTotalCents = -1;
    noteStartTime = 0.0f;
    noteMaxPitchTime = 0.0f;
    noteMinPitchTime = 0.0f;

    // Reset accumulation buffer
    vocalAccumCount = 0;
    // this fill is not necessary but why not lol
    std::fill(vocalAccumBuffer.begin(), vocalAccumBuffer.end(), 0.0f);

    // Pre-allocate to avoid real-time allocations during recording
    recNotesVec.reserve(1024);
    {
        std::scoped_lock lock(pitchCurveMutex);
        pitchCurve.first.reserve(8192);
        pitchCurve.second.reserve(8192);
    }

    // Reset pitch detector
    if (pitchDetector) {
        pitchDetector->reset();
    }

    params.vocalToMelody = true;
}

void AudioPluginAudioProcessor::stopRecordingVocal() {
    if (isRecNote) {
        fixateRecordingNote();
    }

    params.vocalToMelody = false;
    currentVocalTotalCents = -1;
    recNoteStartTotalCents = -1;
    recNoteMinTotalCents = -1;
    recNoteMaxTotalCents = -1;
}

// ============================================================================================

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                             juce::MidiBuffer &midiMessages) {
    if (!isActive)
        return;

    std::unique_lock lock(changeInstanceSyncMutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return;
    }

    // =========================== PARTIALS FINDING ===========================
    // Track active midi notes
    for (const auto metadata : midiMessages) {
        const auto message = metadata.getMessage();
        const int noteInd = message.getNoteNumber();
        if (noteInd >= params.num_octaves * 12)
            continue;

        if (message.isNoteOn()) {
            activeMidiNotes[noteInd] = true;
        } else if (message.isNoteOff()) {
            activeMidiNotes[noteInd] = false;
        }
    }

    // If partials finding mode
    if (params.findPartialsMode.load()) {
        // If previously was in piano roll mode
        if (wasPianoRoll) {
            if (params.getTuningType() == Parameters::TuningType::MTS_ESP) {
                pluginInstanceManager->updateFreqs(freqs12EDO);
            }
            partialsFinderBuffer->clear();
            activeMidiNotes.fill(false); // just in case
            isRecording = false;
            wasPianoRoll = false;
        }

        int numActNotes = std::count(activeMidiNotes.begin(), activeMidiNotes.end(), true);

        if (isRecording) {
            if (numActNotes == 0) {
                partialsFinderBuffer->addSamples(buffer);
                startPartialsFinding();
            } else if (numActNotes == 1) {
                if (activeMidiNotes[recordingMidiNote]) {
                    partialsFinderBuffer->addSamples(buffer);
                } else {
                    partialsFinderBuffer->addSamples(buffer);
                    startPartialsFinding();
                    tryStartRecording();
                }
            } else {
                partialsFinderBuffer->addSamples(buffer);
                startPartialsFinding();
            }
        } else {
            // Can catch a note that has not just started playing, but for example,
            //   started playing earlier with other notes that then stopped playing.
            //   But in any case, it is then the user's fault.
            if (numActNotes == 1) {
                tryStartRecording();
            }
        }

        return;
    } else {
        // If piano roll mode right after partials finding mode
        if (!wasPianoRoll) {
            prepareNotes();
            wasPianoRoll = true;
        }
    }
    // ========================================================================

    juce::ScopedNoDenormals noDenormals;
    midiMessages.clear(); // Clear incoming MIDI

    int numSamples = buffer.getNumSamples();
    double sampleRate = getSampleRate();
    auto audioPlayHead = getPlayHead();
    if (audioPlayHead != nullptr) {
        auto positionInfo = audioPlayHead->getPosition();
        if (positionInfo.hasValue()) {
            auto timeSig = positionInfo->getTimeSignature().orFallback(
                juce::AudioPlayHead::TimeSignature{4, 4});
            numerator = timeSig.numerator;
            denominator = timeSig.denominator;
            double beatsPerBar = static_cast<double>(numerator) / (denominator / 4.0);
            double ppqPosition = positionInfo->getPpqPosition().orFallback(0.0);
            playHeadTime = ppqPosition / beatsPerBar;

            bpm = positionInfo->getBpm().orFallback(120.0);
            double secondsPerBeat = 60.0 / bpm;
            double samplesPerBeat = secondsPerBeat * sampleRate;
            double beatsInBlock = numSamples / samplesPerBeat;
            double barsInBlock = beatsInBlock / beatsPerBar;
            bool isPlaying = positionInfo->getIsPlaying();

            // ============================= VOCAL TO MELODY ============================
            if (params.vocalToMelody) {
                if (isPlaying) {
                    pitchTime = playHeadTime - vocalFFTSize / samplesPerBeat / beatsPerBar;
                    processVocalInput(buffer, numSamples, sampleRate);
                } else if (wasPlaying) {
                    if (isRecNote) {
                        {
                            std::scoped_lock lock(recNoteMutex);
                            pitchTime = recNote.time + recNote.duration;
                        }
                        fixateRecordingNote();

                        // Add gap to pitch curve
                        std::scoped_lock lock(pitchCurveMutex);
                        pitchCurve.first.push_back(pitchTime + 1e-4);
                        pitchCurve.second.push_back(-1);
                    }

                    // Reset accumulation buffer
                    vocalAccumCount = 0;

                    // Reset pitch detector
                    if (pitchDetector) {
                        pitchDetector->reset();
                    }
                }
            }
            // ==========================================================================

            if (params.getTuningType() == Parameters::TuningType::MPE) {
                // ================================================================================
                // =                                  USING MPE                                   =
                // ================================================================================

                centsPerBendMPE = params.semiBendRangeMPE * 100.0 / 8192;
                // Taking into account A4 freq (default is 440 Hz)
                corrTotalCentsMPE = 1200 * log2(params.A4Freq / 440.0);

                {
                    std::scoped_lock lock(manPlNotesMutex);
                    // Stop playing unexisting notes (manually played)
                    for (auto it = manPlNoteToChAndMidiNoteMPE.begin();
                         it != manPlNoteToChAndMidiNoteMPE.end();) {
                        const auto &[totalCents, chAndMidiNote] = *it;
                        if (!manuallyPlayedNotes.contains(totalCents)) {
                            juce::MidiMessage noteOff = juce::MidiMessage::noteOff(
                                chAndMidiNote.first, chAndMidiNote.second);
                            midiMessages.addEvent(noteOff, 0);
                            channelsManagerMPE->noteReleasedMPE(chAndMidiNote.first);
                            it = manPlNoteToChAndMidiNoteMPE.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }

                {
                    // TODO: if it will cause audio glitches,
                    //       change the approach to an intermediate buffer for notes
                    std::scoped_lock lock(notesMutex);
                    // Stop playing unexisting notes (from piano roll)
                    if (isPlaying) {
                        for (auto it = noteToChAndMidiNoteMPE.begin();
                             it != noteToChAndMidiNoteMPE.end();) {
                            const auto &[key, chAndMidiNote] = *it;
                            int i = key.first;
                            int totalCents = key.second;
                            bool stopPlayingThisNote = false;

                            if (i >= notes.size()) {
                                stopPlayingThisNote = true;
                            } else {
                                const Note &note = notes[i];
                                if (note.octave * 1200 + note.cents != totalCents) {
                                    stopPlayingThisNote = true;
                                } else {
                                    stopPlayingThisNote =
                                        (playHeadTime < note.time ||
                                         playHeadTime > note.time + note.duration);
                                }
                            }

                            if (stopPlayingThisNote) {
                                juce::MidiMessage noteOff = juce::MidiMessage::noteOff(
                                    chAndMidiNote.first, chAndMidiNote.second);
                                midiMessages.addEvent(noteOff, 0);
                                delCurrPlayedNotesTotalCentsMPE(totalCents);
                                channelsManagerMPE->noteReleasedMPE(chAndMidiNote.first);
                                it = noteToChAndMidiNoteMPE.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    } else if (wasPlaying) {
                        for (const auto &[key, chAndMidiNote] : noteToChAndMidiNoteMPE) {
                            juce::MidiMessage noteOff = juce::MidiMessage::noteOff(
                                chAndMidiNote.first, chAndMidiNote.second);
                            midiMessages.addEvent(noteOff, 0);
                            delCurrPlayedNotesTotalCentsMPE(key.second);
                            channelsManagerMPE->noteReleasedMPE(chAndMidiNote.first);
                        }
                        noteToChAndMidiNoteMPE.clear();
                    }

                    // Stop playing auditioning notes from piano roll
                    if (isAuditioning) {
                        for (auto it = auditionNoteToChAndMidiNoteMPE.begin();
                             it != auditionNoteToChAndMidiNoteMPE.end();) {
                            const auto &[key, chAndMidiNote] = *it;
                            int i = key.first;
                            int totalCents = key.second;
                            bool stopPlayingThisNote = false;

                            if (i >= notes.size()) {
                                stopPlayingThisNote = true;
                            } else {
                                const Note &note = notes[i];
                                if (note.octave * 1200 + note.cents != totalCents) {
                                    stopPlayingThisNote = true;
                                } else {
                                    stopPlayingThisNote =
                                        (auditionTime < note.time ||
                                         auditionTime >= note.time + note.duration);
                                }
                            }

                            if (stopPlayingThisNote) {
                                juce::MidiMessage noteOff = juce::MidiMessage::noteOff(
                                    chAndMidiNote.first, chAndMidiNote.second);
                                midiMessages.addEvent(noteOff, 0);
                                delCurrPlayedNotesTotalCentsMPE(totalCents);
                                channelsManagerMPE->noteReleasedMPE(chAndMidiNote.first);
                                it = auditionNoteToChAndMidiNoteMPE.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    } else if (stopAuditioning) {
                        for (const auto &[key, chAndMidiNote] : auditionNoteToChAndMidiNoteMPE) {
                            juce::MidiMessage noteOff = juce::MidiMessage::noteOff(
                                chAndMidiNote.first, chAndMidiNote.second);
                            midiMessages.addEvent(noteOff, 0);
                            delCurrPlayedNotesTotalCentsMPE(key.second);
                            channelsManagerMPE->noteReleasedMPE(chAndMidiNote.first);
                        }
                        auditionNoteToChAndMidiNoteMPE.clear();
                        stopAuditioning = false;
                    }

                    int midiNote, bendMPE;

                    // Play auditioning notes from piano roll
                    if (isAuditioning && auditionChanged) {
                        // Note bend
                        for (const auto &[noteIndAndTotCents, chAndMidiNote] :
                             auditionNoteToChAndMidiNoteMPE) {
                            bendMPE = calcBendMPE(notes[noteIndAndTotCents.first], auditionTime);
                            juce::MidiMessage pitchBend =
                                juce::MidiMessage::pitchWheel(chAndMidiNote.first, bendMPE);
                            midiMessages.addEvent(pitchBend, 0);
                        }

                        // Note on
                        for (int i = 0; i < notes.size(); ++i) {
                            const Note &note = notes[i];
                            if ((note.time <= auditionTime) &&
                                (auditionTime < note.time + note.duration)) {
                                int totalCents = note.octave * 1200 + note.cents;
                                if (!auditionNoteToChAndMidiNoteMPE.contains({i, totalCents})) {
                                    std::tie(midiNote, bendMPE) =
                                        calcMidiNoteAndBendMPE(totalCents);
                                    if (note.bend != 0) {
                                        bendMPE = calcBendMPE(note, auditionTime);
                                    }
                                    int ch = channelsManagerMPE->allocateChannelMPE(bendMPE,
                                                                                    note.bend != 0);
                                    if (ch != -1) {
                                        pitchesOverflow = false;
                                        juce::MidiMessage pitchBend =
                                            juce::MidiMessage::pitchWheel(ch, bendMPE);
                                        midiMessages.addEvent(pitchBend, 0);
                                        juce::MidiMessage noteOn =
                                            juce::MidiMessage::noteOn(ch, midiNote, note.velocity);
                                        midiMessages.addEvent(noteOn, 0);
                                        auditionNoteToChAndMidiNoteMPE[{i, totalCents}] =
                                            std::make_pair(ch, midiNote);
                                        addCurrPlayedNotesTotalCentsMPE(totalCents);
                                    } else {
                                        pitchesOverflow = true;
                                    }
                                }
                            }
                        }
                        auditionChanged = false;
                    }

                    // Play notes from piano roll
                    if (isPlaying) {
                        // ===================== Note off =====================
                        for (int i = 0; i < notes.size(); ++i) {
                            const Note &note = notes[i];
                            if ((note.time + note.duration >= playHeadTime) &&
                                (note.time + note.duration < playHeadTime + barsInBlock)) {
                                int totalCents = note.octave * 1200 + note.cents;
                                const auto &itNote = noteToChAndMidiNoteMPE.find({i, totalCents});
                                if (itNote != noteToChAndMidiNoteMPE.end()) {
                                    int noteOffSample = static_cast<int>(floor(
                                        numSamples * (note.time + note.duration - playHeadTime) /
                                        barsInBlock));
                                    const auto &chAndMidiNote = itNote->second;
                                    juce::MidiMessage noteOff = juce::MidiMessage::noteOff(
                                        chAndMidiNote.first, chAndMidiNote.second);
                                    midiMessages.addEvent(noteOff, noteOffSample);
                                    delCurrPlayedNotesTotalCentsMPE(totalCents);

                                    if (params.resetPitchBendOnNoteOff && (note.bend != 0)) {
                                        std::tie(midiNote, bendMPE) =
                                            calcMidiNoteAndBendMPE(totalCents);
                                        juce::MidiMessage pitchBend = juce::MidiMessage::pitchWheel(
                                            chAndMidiNote.first, bendMPE);
                                        midiMessages.addEvent(pitchBend, noteOffSample);
                                    }

                                    channelsManagerMPE->noteReleasedMPE(chAndMidiNote.first);
                                    noteToChAndMidiNoteMPE.erase(itNote);
                                }
                            }
                        }
                        // ===================== Note on =====================
                        const bool chaseMIDINotes =
                            GlobalSettings::getInstance().getChaseMIDINotes();
                        for (int i = 0; i < notes.size(); ++i) {
                            const Note &note = notes[i];
                            int totalCents = note.octave * 1200 + note.cents;
                            bool rightBorderCond =
                                chaseMIDINotes
                                    ? playHeadTime <= note.time + note.duration - barsInBlock
                                    : playHeadTime <= note.time;
                            if ((note.time - barsInBlock < playHeadTime) && rightBorderCond &&
                                (!noteToChAndMidiNoteMPE.contains({i, totalCents}))) {
                                std::tie(midiNote, bendMPE) = calcMidiNoteAndBendMPE(totalCents);
                                int ch =
                                    channelsManagerMPE->allocateChannelMPE(bendMPE, note.bend != 0);
                                if (ch != -1) {
                                    pitchesOverflow = false;
                                    int noteOnSample = 0;
                                    if (playHeadTime < note.time) {
                                        noteOnSample = static_cast<int>(ceil(
                                            numSamples * (note.time - playHeadTime) / barsInBlock));
                                    }
                                    juce::MidiMessage pitchBend =
                                        juce::MidiMessage::pitchWheel(ch, bendMPE);
                                    midiMessages.addEvent(pitchBend, noteOnSample);
                                    juce::MidiMessage noteOn =
                                        juce::MidiMessage::noteOn(ch, midiNote, note.velocity);
                                    midiMessages.addEvent(noteOn, noteOnSample);
                                    noteToChAndMidiNoteMPE[{i, totalCents}] =
                                        std::make_pair(ch, midiNote);
                                    addCurrPlayedNotesTotalCentsMPE(totalCents);
                                } else {
                                    pitchesOverflow = true;
                                }
                            }
                        }
                        // ===================== Note bend =====================
                        for (int i = 0; i < notes.size(); ++i) {
                            const Note &note = notes[i];
                            if ((note.bend != 0) && (note.time < playHeadTime) &&
                                (playHeadTime <= note.time + note.duration)) {
                                int totalCents = note.octave * 1200 + note.cents;
                                const auto &itNote = noteToChAndMidiNoteMPE.find({i, totalCents});
                                if (itNote != noteToChAndMidiNoteMPE.end()) {
                                    const auto &chAndMidiNote = itNote->second;
                                    int bendMPE = calcBendMPE(note, playHeadTime);
                                    juce::MidiMessage pitchBend =
                                        juce::MidiMessage::pitchWheel(chAndMidiNote.first, bendMPE);
                                    midiMessages.addEvent(pitchBend, 0);
                                }
                            }
                        }
                    }
                }

                // Start playing manually played notes
                {
                    std::scoped_lock lock(manPlNotesMutex);
                    int midiNote, bendMPE;
                    for (const auto &[totalCents, velocity] : manuallyPlayedNotes) {
                        if (!manPlNoteToChAndMidiNoteMPE.contains(totalCents)) {
                            std::tie(midiNote, bendMPE) = calcMidiNoteAndBendMPE(totalCents);
                            int ch = channelsManagerMPE->allocateChannelMPE(bendMPE, false);
                            if (ch != -1) {
                                pitchesOverflow = false;
                                juce::MidiMessage pitchBend =
                                    juce::MidiMessage::pitchWheel(ch, bendMPE);
                                midiMessages.addEvent(pitchBend, 0);
                                juce::MidiMessage noteOn =
                                    juce::MidiMessage::noteOn(ch, midiNote, velocity);
                                midiMessages.addEvent(noteOn, 0);
                                manPlNoteToChAndMidiNoteMPE[totalCents] =
                                    std::make_pair(ch, midiNote);
                            } else {
                                pitchesOverflow = true;
                            }
                        }
                    }
                }

                wasPlaying = isPlaying;

            } else if (params.getTuningType() == Parameters::MTS_ESP) {
                // ================================================================================
                // =                                USING MTS-ESP                                 =
                // ================================================================================

                // TODO: if it causes glitches,
                //       change the approach to an intermediate buffers for things that are
                //       changed in prepareToPlay(). Also then use buffers for notes and
                //       manPlNotes, so all data  will be consistent
                std::scoped_lock lock(prepareNotesMutex);

                {
                    std::scoped_lock lock(manPlNotesMutex);
                    // Stop playing unexisting notes (manually played)
                    for (auto it = manPlNoteToMidiNoteMTS.begin();
                         it != manPlNoteToMidiNoteMTS.end();) {
                        if (!manuallyPlayedNotes.contains(it->first)) {
                            juce::MidiMessage noteOff =
                                juce::MidiMessage::noteOff(params.channelIndex + 1, it->second);
                            midiMessages.addEvent(noteOff, 0);
                            currPlayedNotesIndexes.erase(it->second);
                            it = manPlNoteToMidiNoteMTS.erase(it);
                        } else {
                            ++it;
                        }
                    }

                    // Start playing manually played notes
                    for (const auto &[totalCents, velocity] : manuallyPlayedNotes) {
                        if (!manPlNoteToMidiNoteMTS.contains(totalCents)) {
                            double noteFreq = getFreqFromTotalCents(totalCents);
                            int noteInd = findFreqInd(noteFreq);
                            if (noteInd != -1) {
                                juce::MidiMessage noteOn = juce::MidiMessage::noteOn(
                                    params.channelIndex + 1, noteInd, velocity);
                                midiMessages.addEvent(noteOn, 0);
                                currPlayedNotesIndexes.insert(noteInd);
                                manPlNoteToMidiNoteMTS[totalCents] = noteInd;
                            }
                        }
                    }
                }

                {
                    std::scoped_lock lock(notesMutex);
                    // Stop playing auditioning notes from piano roll
                    if (isAuditioning) {
                        for (auto it = auditionNoteToMidiNoteMTS.begin();
                             it != auditionNoteToMidiNoteMTS.end();) {
                            int i = it->first.first;
                            int totalCents = it->first.second;
                            bool stopPlayingThisNote = true;
                            if (i < notes.size()) {
                                const Note &note = notes[i];
                                int noteTotalCents = note.octave * 1200 + note.cents;
                                if ((noteTotalCents == totalCents) && (auditionTime >= note.time) &&
                                    (auditionTime < note.time + note.duration)) {
                                    stopPlayingThisNote = false;
                                }
                            }
                            if (stopPlayingThisNote) {
                                int noteInd = it->second;
                                juce::MidiMessage noteOff =
                                    juce::MidiMessage::noteOff(params.channelIndex + 1, noteInd);
                                midiMessages.addEvent(noteOff, 0);
                                currPlayedNotesIndexes.erase(noteInd);
                                currPlayedNotesTotalCents.erase(totalCents);
                                it = auditionNoteToMidiNoteMTS.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    } else if (stopAuditioning) {
                        for (const auto &[noteIndAndTotCents, noteInd] :
                             auditionNoteToMidiNoteMTS) {
                            juce::MidiMessage noteOff =
                                juce::MidiMessage::noteOff(params.channelIndex + 1, noteInd);
                            midiMessages.addEvent(noteOff, 0);
                            currPlayedNotesIndexes.erase(noteInd);
                            currPlayedNotesTotalCents.erase(noteIndAndTotCents.second);
                        }
                        auditionNoteToMidiNoteMTS.clear();
                        stopAuditioning = false;
                    }

                    // Play auditioning notes from piano roll
                    if (isAuditioning && auditionChanged) {
                        bool needUpdateFreqs = false;
                        for (int i = 0; i < notes.size(); ++i) {
                            const Note &note = notes[i];
                            if ((note.time <= auditionTime) &&
                                (auditionTime < note.time + note.duration)) {
                                int totalCents = note.octave * 1200 + note.cents;
                                auto it = auditionNoteToMidiNoteMTS.find({i, totalCents});
                                if (it == auditionNoteToMidiNoteMTS.end()) {
                                    int noteInd = notesIndexes[i];
                                    if (noteInd == -1)
                                        continue;
                                    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(
                                        params.channelIndex + 1, noteInd, note.velocity);
                                    midiMessages.addEvent(noteOn, 0);
                                    currPlayedNotesIndexes.insert(noteInd);
                                    currPlayedNotesTotalCents.insert(totalCents);
                                    auditionNoteToMidiNoteMTS[{i, totalCents}] = noteInd;
                                    if (note.bend != 0) {
                                        freqs[noteInd] = getNoteFreqAtTime(note, auditionTime);
                                        needUpdateFreqs = true;
                                    }
                                } else if (note.bend != 0) {
                                    freqs[it->second] = getNoteFreqAtTime(note, auditionTime);
                                    needUpdateFreqs = true;
                                }
                            }
                        }
                        if (needUpdateFreqs) {
                            pluginInstanceManager->updateFreqs(freqs);
                        }
                        auditionChanged = false;
                    }

                    // Play notes from piano roll
                    if (isPlaying) {
                        // =======================================
                        for (int i = 0; i < notes.size(); ++i) {
                            const Note &note = notes[i];
                            // Note off
                            if ((note.time + note.duration >=
                                 playHeadTime) && /*currPlayedNotesTotalCents.contains(note.octave*1200+note.cents)*/
                                (note.time + note.duration < playHeadTime + barsInBlock)) {
                                const int noteInd = notesIndexes[i];
                                juce::MidiMessage noteOff =
                                    juce::MidiMessage::noteOff(params.channelIndex + 1, noteInd);
                                midiMessages.addEvent(
                                    noteOff,
                                    static_cast<int>(floor(
                                        numSamples * (note.time + note.duration - playHeadTime) /
                                        barsInBlock)));
                                currPlayedNotesTotalCents.erase(note.octave * 1200 + note.cents);
                            }
                        }
                        bool needUpdateFreqs = false;
                        // =======================================
                        for (int i = 0; i < notes.size(); ++i) {
                            const Note &note = notes[i];
                            // PRE Note on
                            if ((note.time >= playHeadTime) &&
                                (note.time < playHeadTime + 2 * barsInBlock)) {
                                // If note was bending - update frequency (and the start of
                                // note will be without pitch leap)
                                const int noteInd = notesIndexes[i];
                                if (beforeBendTotalCents[noteInd] != -1) {
                                    freqs[noteInd] =
                                        getFreqFromTotalCents(note.octave * 1200 + note.cents);
                                    needUpdateFreqs = true;
                                }
                            }
                        }
                        // =======================================
                        const bool chaseMIDINotes =
                            GlobalSettings::getInstance().getChaseMIDINotes();
                        for (int i = 0; i < notes.size(); ++i) {
                            const Note &note = notes[i];
                            // Note on
                            bool rightBorderCond =
                                chaseMIDINotes
                                    ? playHeadTime <= note.time + note.duration - barsInBlock
                                    : playHeadTime <= note.time;
                            if ((note.time - barsInBlock < playHeadTime) && rightBorderCond &&
                                !currPlayedNotesTotalCents.contains(note.octave * 1200 +
                                                                    note.cents)) {
                                const int noteInd = notesIndexes[i];
                                juce::MidiMessage noteOn = juce::MidiMessage::noteOn(
                                    params.channelIndex + 1, noteInd, note.velocity);
                                int noteOnSample = 0;
                                if (playHeadTime < note.time) {
                                    noteOnSample = static_cast<int>(ceil(
                                        numSamples * (note.time - playHeadTime) / barsInBlock));
                                }
                                midiMessages.addEvent(noteOn, noteOnSample);
                                currPlayedNotesTotalCents.insert(note.octave * 1200 + note.cents);
                                currPlayedNotesIndexes.insert(noteInd);
                                if (note.bend != 0) {
                                    beforeBendTotalCents[noteInd] = note.octave * 1200 + note.cents;
                                } else {
                                    beforeBendTotalCents[noteInd] = -1;
                                }
                            }
                        }
                        // =======================================
                        for (int i = 0; i < notes.size(); ++i) {
                            const Note &note = notes[i];
                            // Note bend
                            if ((note.bend != 0) && (note.time < playHeadTime) &&
                                (playHeadTime <= note.time + note.duration)) {
                                freqs[notesIndexes[i]] = getNoteFreq(note);
                                needUpdateFreqs = true;
                            }
                        }
                        if (needUpdateFreqs) {
                            pluginInstanceManager->updateFreqs(freqs);
                        }
                    } else {
                        // IF THERE WERE NOTE BENDS AND NOTES DIDN'T END BEFORE WE STOPPED
                        //    PLAYBACK (this isn't necessarily to do)
                        if (wasPlaying) {
                            bool wasBend = false;
                            for (int i = 0; i < 128; ++i) {
                                const int totCents = beforeBendTotalCents[i];
                                if (totCents != -1) {
                                    freqs[i] = getFreqFromTotalCents(totCents);
                                    beforeBendTotalCents[i] = -1;
                                    wasBend = true;
                                }
                            }
                            if (wasBend) {
                                pluginInstanceManager->updateFreqs(freqs);
                            }
                        }
                    }
                    wasPlaying = isPlaying;

                    // Stop playing unexisting notes
                    for (auto it = currPlayedNotesIndexes.begin();
                         it != currPlayedNotesIndexes.end();) {
                        int ind = *it;
                        bool thereStillExistsThisNote = false;
                        if (isPlaying) {
                            for (int i = 0; i < notes.size(); ++i) {
                                const Note &note = notes[i];
                                if ((notesIndexes[i] == ind) &&
                                    (note.time < playHeadTime + barsInBlock) &&
                                    (note.time + note.duration >= playHeadTime + barsInBlock)) {
                                    // Check if the note's base pitch matches (without bend)
                                    int noteTotalCents = note.octave * 1200 + note.cents;
                                    int freqsTotalCents;
                                    if (beforeBendTotalCents[ind] != -1) {
                                        freqsTotalCents = beforeBendTotalCents[ind];
                                    } else {
                                        freqsTotalCents = getTotalCentsFromFreq(freqs[ind]);
                                    }
                                    if (noteTotalCents == freqsTotalCents) {
                                        thereStillExistsThisNote = true;
                                        break;
                                    }
                                }
                            }
                        }
                        if (!thereStillExistsThisNote) {
                            for (const auto &[_, midiNoteInd] : manPlNoteToMidiNoteMTS) {
                                if (midiNoteInd == ind) {
                                    thereStillExistsThisNote = true;
                                    break;
                                }
                            }
                        }
                        if (!thereStillExistsThisNote) {
                            for (const auto &[key, midiNoteInd] : auditionNoteToMidiNoteMTS) {
                                if (midiNoteInd == ind) {
                                    thereStillExistsThisNote = true;
                                    break;
                                }
                            }
                        }
                        if (thereStillExistsThisNote) {
                            ++it;
                        } else {
                            juce::MidiMessage noteOff =
                                juce::MidiMessage::noteOff(params.channelIndex + 1, ind);
                            midiMessages.addEvent(noteOff, 0);
                            int totalCents;
                            if (beforeBendTotalCents[ind] != -1) {
                                totalCents = beforeBendTotalCents[ind];
                            } else {
                                totalCents = getTotalCentsFromFreq(freqs[ind]);
                            }
                            currPlayedNotesTotalCents.erase(totalCents);
                            it = currPlayedNotesIndexes.erase(it);
                        }
                    }
                }
            }
        }
    }

    buffer.clear();
}

bool AudioPluginAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor() {
    return new AudioPluginAudioProcessorEditor(*this);
}

// TODO: THERE IS NO MUTEX IN PARAMETERS AND NOT ALL FIELDS IN PARAMETERS ARE ATOMIC,
//       SO THIS METHOD CAN CAUSE DATA RACE IF RUNS NOT IN MESSAGE THREAD!
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    juce::ValueTree state("XenRollState");
    state.setProperty("version", 1, nullptr);
    auto paramsTree = state.getOrCreateChildWithName("Parameters", nullptr);

    // Basic params
    paramsTree.setProperty("editorWidth", params.editorWidth, nullptr);
    paramsTree.setProperty("editorHeight", params.editorHeight, nullptr);
    paramsTree.setProperty("numBars", params.get_num_bars(), nullptr);
    paramsTree.setProperty("numBeats", params.num_beats, nullptr);
    paramsTree.setProperty("numSubdivs", params.num_subdivs, nullptr);
    paramsTree.setProperty("startOctave", params.start_octave, nullptr);
    paramsTree.setProperty("a4Freq", params.A4Freq.load(), nullptr);
    paramsTree.setProperty("noteRectHeightCoef", params.noteRectHeightCoef, nullptr);
    paramsTree.setProperty("constNoteRectHeight", params.constNoteRectHeight, nullptr);

    // Tuning & instances sync
    paramsTree.setProperty("tuningType", static_cast<int>(params.getGlobalTuningType()), nullptr);

    // For MPE:
    paramsTree.setProperty("instanceId", params.instanceId, nullptr);
    auto ghostInstTree = paramsTree.getOrCreateChildWithName("GhostNotesInstIds", nullptr);
    for (const int id : params.ghostNotesInstIds) {
        juce::ValueTree idNode("Instance");
        idNode.setProperty("v", id, nullptr);
        ghostInstTree.appendChild(idNode, nullptr);
    }
    paramsTree.setProperty("resetPitchBendOnNoteOff", params.resetPitchBendOnNoteOff.load(),
                           nullptr);
    paramsTree.setProperty("semiBendRangeMPE", params.semiBendRangeMPE.load(), nullptr);
    paramsTree.setProperty("channelsEconomyModeMPE", params.channelsEconomyModeMPE, nullptr);

    // For MTS-ESP:
    paramsTree.setProperty("channelIndex", params.channelIndex, nullptr);
    auto ghostChTree = paramsTree.getOrCreateChildWithName("GhostNotesChannels", nullptr);
    for (const int ch : params.ghostNotesChannels) {
        juce::ValueTree chNode("Channel");
        chNode.setProperty("v", ch, nullptr);
        ghostChTree.appendChild(chNode, nullptr);
    }

    // Zones
    auto zonesTree = paramsTree.getOrCreateChildWithName("Zones", nullptr);
    for (const float zp : params.zones.getZonesPoints()) {
        juce::ValueTree pt("Point");
        pt.setProperty("v", zp, nullptr);
        zonesTree.appendChild(pt, nullptr);
    }
    for (const bool zof : params.zones.getZonesOnOff()) {
        juce::ValueTree onOff("OnOff");
        onOff.setProperty("v", zof, nullptr);
        zonesTree.appendChild(onOff, nullptr);
    }

    // Intellectual / Partials
    paramsTree.setProperty("findPartialsFFTSize", params.findPartialsFFTSize.load(), nullptr);
    paramsTree.setProperty("findPartialsdBThreshold", params.findPartialsdBThreshold.load(),
                           nullptr);
    paramsTree.setProperty("findPartialsStrat", static_cast<int>(params.findPartialsStrat.load()),
                           nullptr);
    auto partialsTree = paramsTree.getOrCreateChildWithName("TonesPartials", nullptr);
    for (const auto &[totalCents, partials] : params.get_tonesPartials()) {
        juce::ValueTree tone("Tone");
        tone.setProperty("totalCents", totalCents, nullptr);
        for (const auto &[freq, amp] : partials) {
            juce::ValueTree partial("Partial");
            partial.setProperty("freq", freq, nullptr);
            partial.setProperty("amp", amp, nullptr);
            tone.appendChild(partial, nullptr);
        }
        partialsTree.appendChild(tone, nullptr);
    }
    paramsTree.setProperty("roughCompactFrac", params.roughCompactFrac, nullptr);
    paramsTree.setProperty("dissonancePow", params.dissonancePow, nullptr);
    paramsTree.setProperty("pitchMemoryTVvalForZeroHV", params.pitchMemoryTVvalForZeroHV, nullptr);
    paramsTree.setProperty("pitchMemoryTVaddInfluence", params.pitchMemoryTVaddInfluence, nullptr);
    paramsTree.setProperty("pitchMemoryTVminNonzero", params.pitchMemoryTVminNonzero, nullptr);
    paramsTree.setProperty("pitchMemoryShowOnlyHarmonicity", params.pitchMemoryShowOnlyHarmonicity,
                           nullptr);

    // Theme
    paramsTree.setProperty("themeType", static_cast<int>(params.themeType), nullptr);

    // Notes
    auto notesTree = state.getOrCreateChildWithName("Notes", nullptr);
    {
        std::scoped_lock lock(notesMutex);
        for (const auto &note : notes) {
            juce::ValueTree noteNode("Note");
            noteNode.setProperty("octave", note.octave, nullptr);
            noteNode.setProperty("cents", note.cents, nullptr);
            noteNode.setProperty("time", note.time, nullptr);
            noteNode.setProperty("isSelected", note.isSelected, nullptr);
            noteNode.setProperty("duration", note.duration, nullptr);
            noteNode.setProperty("velocity", note.velocity, nullptr);
            noteNode.setProperty("bend", note.bend, nullptr);
            notesTree.appendChild(noteNode, nullptr);
        }
    }

    // RatioMarks
    paramsTree.setProperty("autoCorrectRatiosMarks", params.autoCorrectRatiosMarks, nullptr);
    paramsTree.setProperty("maxDenRatiosMarks", params.maxDenRatiosMarks, nullptr);
    paramsTree.setProperty("goodEnoughErrorRatiosMarks", params.goodEnoughErrorRatiosMarks,
                           nullptr);
    auto ratiosTree = paramsTree.getOrCreateChildWithName("RatioMarks", nullptr);
    for (const auto &rm : params.ratiosMarks) {
        juce::ValueTree rmNode("RatioMark");
        auto [lowerPitch, higherPitch] = rm.getPitches();
        auto [lowerPitchNoteInd, higherPitchNoteInd] = rm.getNoteInds();
        if (lowerPitch > higherPitch) {
            std::swap(lowerPitch, higherPitch);
            std::swap(lowerPitchNoteInd, higherPitchNoteInd);
        }
        rmNode.setProperty("lowerKeyTotalCents", lowerPitch, nullptr);
        rmNode.setProperty("higherKeyTotalCents", higherPitch, nullptr);
        rmNode.setProperty("time", rm.getTime(), nullptr);
        rmNode.setProperty("lowerNoteIndex", lowerPitchNoteInd, nullptr);
        rmNode.setProperty("higherNoteIndex", higherPitchNoteInd, nullptr);
        ratiosTree.appendChild(rmNode, nullptr);
    }

    // Vocal to melody
    paramsTree.setProperty("vocalToMelodyGenCurve", params.vocalToMelodyGenCurve.load(), nullptr);
    paramsTree.setProperty("vocalToMelodyGenNotes", params.vocalToMelodyGenNotes.load(), nullptr);
    paramsTree.setProperty("vocalToMelodyMinNoteDuration",
                           params.vocalToMelodyMinNoteDuration.load(), nullptr);
    paramsTree.setProperty("vocalToMelodyDCents", params.vocalToMelodyDCents.load(), nullptr);
    paramsTree.setProperty("vocalToMelodyKeySnap", params.vocalToMelodyKeySnap.load(), nullptr);
    paramsTree.setProperty("vocalToMelodyMakeBends", params.vocalToMelodyMakeBends.load(), nullptr);

    // Editor state
    paramsTree.setProperty("lastDuration", params.lastDuration, nullptr);
    paramsTree.setProperty("lastVelocity", params.lastVelocity, nullptr);
    paramsTree.setProperty("lastViewPosX", params.lastViewPos.getX(), nullptr);
    paramsTree.setProperty("lastViewPosY", params.lastViewPos.getY(), nullptr);
    paramsTree.setProperty("octaveHeightPx", params.octave_height_px, nullptr);
    paramsTree.setProperty("barWidthPx", params.bar_width_px, nullptr);

    // Misc
    paramsTree.setProperty("maxChordDtimeClockDiagram", params.maxChordDtimeClockDiagram, nullptr);
    paramsTree.setProperty("showGhostNotesKeys", params.showGhostNotesKeys, nullptr);

    juce::MemoryOutputStream stream(destData, false);
    stream.writeInt(MAGIC_NUMBER); // MAGIC NUMBER (for ValueTree)
    state.writeToStream(stream);
}

// TODO: THERE IS NO MUTEX IN PARAMETERS AND NOT ALL FIELDS IN PARAMETERS ARE ATOMIC,
//       SO THIS METHOD CAN CAUSE DATA RACE IF RUNS NOT IN MESSAGE THREAD!
void AudioPluginAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    if (data == nullptr || sizeInBytes <= 0) {
        std::scoped_lock lock(notesMutex);
        notes.clear();
        return;
    }

    juce::MemoryInputStream stream(data, sizeInBytes, false);

    // CHECKING MAGIC NUMBER (for ValueTree)
    int magicNumber = stream.readInt();
    if (magicNumber != MAGIC_NUMBER) {
        legacySetStateInformation(data, sizeInBytes);
        return;
    }

    auto state = juce::ValueTree::readFromStream(stream);

    if (!state.isValid() || !state.hasType("XenRollState")) {
        legacySetStateInformation(data, sizeInBytes);
        return;
    }

    auto paramsTree = state.getChildWithName("Parameters");
    if (!paramsTree.isValid()) {
        std::scoped_lock lock(notesMutex);
        notes.clear();
        return;
    }

    // Basic params
    params.editorWidth = paramsTree.getProperty("editorWidth", params.editorWidth);
    params.editorHeight = paramsTree.getProperty("editorHeight", params.editorHeight);
    int numBars = paramsTree.getProperty("numBars", params.get_num_bars());
    params.set_num_bars(numBars);
    params.num_beats = paramsTree.getProperty("numBeats", params.num_beats);
    params.num_subdivs = paramsTree.getProperty("numSubdivs", params.num_subdivs);
    params.start_octave = paramsTree.getProperty("startOctave", params.start_octave);
    params.A4Freq.store(
        static_cast<double>(paramsTree.getProperty("a4Freq", params.A4Freq.load())));
    params.noteRectHeightCoef =
        static_cast<float>(paramsTree.getProperty("noteRectHeightCoef", params.noteRectHeightCoef));
    params.constNoteRectHeight = static_cast<bool>(
        paramsTree.getProperty("constNoteRectHeight", params.constNoteRectHeight));

    // Tuning & instances sync
    auto newTuningType = static_cast<Parameters::TuningType>(static_cast<int>(
        paramsTree.getProperty("tuningType", static_cast<int>(params.getGlobalTuningType()))));
    if (newTuningType != params.getTuningType()) {
        changeInstanceSync(newTuningType);
    }

    // For MPE:
    int desiredInstanceId = paramsTree.getProperty("instanceId", params.instanceId);
    if ((params.getTuningType() == Parameters::TuningType::MPE) &&
        (desiredInstanceId != params.instanceId) && (desiredInstanceId != -1)) {
        notesSharingMPE->changeInstanceId(desiredInstanceId);
        params.instanceId = notesSharingMPE->getInstanceId();
    }
    auto ghostInstTree = paramsTree.getChildWithName("GhostNotesInstIds");
    params.ghostNotesInstIds.clear();
    if (ghostInstTree.isValid()) {
        for (auto idNode : ghostInstTree) {
            if (idNode.hasType("Instance")) {
                params.ghostNotesInstIds.insert(static_cast<int>(idNode.getProperty("v", 0)));
            }
        }
    }
    params.resetPitchBendOnNoteOff = static_cast<bool>(
        paramsTree.getProperty("resetPitchBendOnNoteOff", params.resetPitchBendOnNoteOff.load()));
    params.semiBendRangeMPE = static_cast<int>(
        paramsTree.getProperty("semiBendRangeMPE", params.semiBendRangeMPE.load()));
    params.channelsEconomyModeMPE = static_cast<bool>(
        paramsTree.getProperty("channelsEconomyModeMPE", params.channelsEconomyModeMPE));
    channelsManagerMPE->setEconomyMode(params.channelsEconomyModeMPE);

    // For MTS-ESP:
    int desiredChannelIndex = paramsTree.getProperty("channelIndex", params.channelIndex);
    if ((params.getTuningType() == Parameters::TuningType::MTS_ESP) &&
        (desiredChannelIndex != params.channelIndex) && (desiredChannelIndex != -1)) {
        pluginInstanceManager->changeChannelIndex(desiredChannelIndex);
        params.channelIndex = pluginInstanceManager->getChannelIndex();
    }
    auto ghostChTree = paramsTree.getChildWithName("GhostNotesChannels");
    params.ghostNotesChannels.clear();
    if (ghostChTree.isValid()) {
        for (auto chNode : ghostChTree) {
            if (chNode.hasType("Channel")) {
                params.ghostNotesChannels.insert(static_cast<int>(chNode.getProperty("v", 0)));
            }
        }
    }

    // Zones
    auto zonesTree = paramsTree.getChildWithName("Zones");
    if (zonesTree.isValid()) {
        std::set<float> zonesPoints;
        std::vector<bool> zonesOnOff;
        for (auto child : zonesTree) {
            if (child.hasType("Point")) {
                zonesPoints.insert(static_cast<float>(child.getProperty("v", 0.0f)));
            } else if (child.hasType("OnOff")) {
                zonesOnOff.push_back(static_cast<bool>(child.getProperty("v", true)));
            }
        }
        params.zones = Zones(static_cast<float>(numBars), zonesPoints, zonesOnOff);
    }

    // Partials / Intellectual
    params.findPartialsFFTSize.store(static_cast<int>(
        paramsTree.getProperty("findPartialsFFTSize", params.findPartialsFFTSize.load())));
    params.findPartialsdBThreshold.store(static_cast<float>(
        paramsTree.getProperty("findPartialsdBThreshold", params.findPartialsdBThreshold.load())));
    params.findPartialsStrat.store(
        static_cast<PartialsFindPosStrat>(static_cast<int>(paramsTree.getProperty(
            "findPartialsStrat", static_cast<int>(params.findPartialsStrat.load())))));

    auto partialsTree = paramsTree.getChildWithName("TonesPartials");
    if (partialsTree.isValid()) {
        std::map<int, partialsVec> tonesPartials;
        for (auto toneNode : partialsTree) {
            if (toneNode.hasType("Tone")) {
                int totalCents = toneNode.getProperty("totalCents", 0);
                partialsVec partials;
                for (auto partialNode : toneNode) {
                    if (partialNode.hasType("Partial")) {
                        float freq = static_cast<float>(partialNode.getProperty("freq", 0.0f));
                        float amp = static_cast<float>(partialNode.getProperty("amp", 0.0f));
                        partials.emplace_back(freq, amp);
                    }
                }
                tonesPartials[totalCents] = std::move(partials);
            }
        }
        params.set_tonesPartials(tonesPartials);
    }

    params.roughCompactFrac =
        static_cast<float>(paramsTree.getProperty("roughCompactFrac", params.roughCompactFrac));
    params.dissonancePow =
        static_cast<float>(paramsTree.getProperty("dissonancePow", params.dissonancePow));
    params.pitchMemoryTVvalForZeroHV = static_cast<float>(
        paramsTree.getProperty("pitchMemoryTVvalForZeroHV", params.pitchMemoryTVvalForZeroHV));
    params.pitchMemoryTVaddInfluence = static_cast<float>(
        paramsTree.getProperty("pitchMemoryTVaddInfluence", params.pitchMemoryTVaddInfluence));
    params.pitchMemoryTVminNonzero = static_cast<float>(
        paramsTree.getProperty("pitchMemoryTVminNonzero", params.pitchMemoryTVminNonzero));
    params.pitchMemoryShowOnlyHarmonicity = static_cast<bool>(paramsTree.getProperty(
        "pitchMemoryShowOnlyHarmonicity", params.pitchMemoryShowOnlyHarmonicity));

    // Theme
    params.themeType = static_cast<Theme::ThemeType>(
        static_cast<int>(paramsTree.getProperty("themeType", static_cast<int>(params.themeType))));
    params.theme.setTheme(params.themeType);

    // Notes
    auto notesTree = state.getChildWithName("Notes");
    {
        std::scoped_lock lock(notesMutex);
        notes.clear();
        if (notesTree.isValid()) {
            for (auto noteNode : notesTree) {
                if (noteNode.hasType("Note")) {
                    Note note;
                    note.octave = noteNode.getProperty("octave", 0);
                    note.cents = noteNode.getProperty("cents", 0);
                    note.time = static_cast<float>(noteNode.getProperty("time", 0.0f));
                    note.isSelected = static_cast<bool>(noteNode.getProperty("isSelected", false));
                    note.duration = static_cast<float>(noteNode.getProperty("duration", 1.0f));
                    note.velocity = static_cast<float>(
                        noteNode.getProperty("velocity", Parameters::defaultVelocity));
                    note.bend = noteNode.getProperty("bend", 0);
                    notes.push_back(note);
                }
            }
        }
    }

    // RatioMarks
    params.autoCorrectRatiosMarks = static_cast<bool>(
        paramsTree.getProperty("autoCorrectRatiosMarks", params.autoCorrectRatiosMarks));
    params.maxDenRatiosMarks =
        paramsTree.getProperty("maxDenRatiosMarks", params.maxDenRatiosMarks);
    params.goodEnoughErrorRatiosMarks =
        paramsTree.getProperty("goodEnoughErrorRatiosMarks", params.goodEnoughErrorRatiosMarks);
    auto ratiosTree = paramsTree.getChildWithName("RatioMarks");
    params.ratiosMarks.clear();
    if (ratiosTree.isValid()) {
        for (auto rmNode : ratiosTree) {
            if (rmNode.hasType("RatioMark")) {
                int lktc = rmNode.getProperty("lowerKeyTotalCents", 0);
                int hktc = rmNode.getProperty("higherKeyTotalCents", 0);
                float t = static_cast<float>(rmNode.getProperty("time", 0.0f));
                int lni = rmNode.getProperty("lowerNoteIndex", -1);
                int hni = rmNode.getProperty("higherNoteIndex", -1);
                RatioMark ratioMark(lktc, hktc, t, &params, lni, hni);
                params.ratiosMarks.push_back(ratioMark);
            }
        }
    }

    // Vocal to melody
    params.vocalToMelodyGenCurve = static_cast<bool>(
        paramsTree.getProperty("vocalToMelodyGenCurve", params.vocalToMelodyGenCurve.load()));
    params.vocalToMelodyGenNotes = static_cast<bool>(
        paramsTree.getProperty("vocalToMelodyGenNotes", params.vocalToMelodyGenNotes.load()));
    params.vocalToMelodyMinNoteDuration = static_cast<float>(paramsTree.getProperty(
        "vocalToMelodyMinNoteDuration", params.vocalToMelodyMinNoteDuration.load()));
    params.vocalToMelodyDCents =
        paramsTree.getProperty("vocalToMelodyDCents", params.vocalToMelodyDCents.load());
    params.vocalToMelodyKeySnap = static_cast<bool>(
        paramsTree.getProperty("vocalToMelodyKeySnap", params.vocalToMelodyKeySnap.load()));
    params.vocalToMelodyMakeBends = static_cast<bool>(
        paramsTree.getProperty("vocalToMelodyMakeBends", params.vocalToMelodyMakeBends.load()));

    // Editor state
    params.lastDuration =
        static_cast<float>(paramsTree.getProperty("lastDuration", params.lastDuration));
    params.lastVelocity =
        static_cast<float>(paramsTree.getProperty("lastVelocity", params.lastVelocity));
    params.lastViewPos.setX(paramsTree.getProperty("lastViewPosX", params.lastViewPos.getX()));
    params.lastViewPos.setY(paramsTree.getProperty("lastViewPosY", params.lastViewPos.getY()));
    params.octave_height_px =
        static_cast<float>(paramsTree.getProperty("octaveHeightPx", params.octave_height_px));
    params.bar_width_px =
        static_cast<float>(paramsTree.getProperty("barWidthPx", params.bar_width_px));

    // Misc
    params.maxChordDtimeClockDiagram = static_cast<float>(
        paramsTree.getProperty("maxChordDtimeClockDiagram", params.maxChordDtimeClockDiagram));
    params.showGhostNotesKeys =
        static_cast<bool>(paramsTree.getProperty("showGhostNotesKeys", params.showGhostNotesKeys));

    // UPDATE NOTES
    {
        std::scoped_lock lock(notesMutex);
        params.stateHistory.push(State(numBars, notes, params.ratiosMarks));
    }
    prepareNotes();
    if (params.getTuningType() == Parameters::TuningType::MPE) {
        notesSharingMPE->updateNotes(notes);
    } else if (params.getTuningType() == Parameters::TuningType::MTS_ESP) {
        pluginInstanceManager->updateNotes(notes);
    }
}

void AudioPluginAudioProcessor::legacySetStateInformation(const void *data, int sizeInBytes) {
    if (data == nullptr || sizeInBytes <= 0) {
        std::scoped_lock lock(notesMutex);
        notes.clear();
        return;
    }
    juce::MemoryInputStream stream(data, sizeInBytes, false);

    // shit-code crutch
    ///< VERSION OF STATE INFORMATION {0, -1, -2, ...}
    int version;
    int smth = stream.readInt();
    if (smth > 0) {
        params.editorWidth = smth;
        version = 0;
    } else {
        version = smth;
        params.editorWidth = stream.readInt();
    }

    // Read simple params
    // params.editorWidth = stream.readInt();
    params.editorHeight = stream.readInt();
    int num_bars = stream.readInt();
    params.set_num_bars(num_bars);
    params.num_beats = stream.readInt();
    params.num_subdivs = stream.readInt();
    params.start_octave = stream.readInt();
    params.A4Freq.store(stream.readDouble());
    params.noteRectHeightCoef = stream.readFloat();

    if (version <= -2) {
        Parameters::TuningType newTuningType =
            static_cast<Parameters::TuningType>(stream.readInt());
        if (newTuningType != params.getTuningType()) {
            changeInstanceSync(newTuningType);
        }
    }

    // Read zones
    std::set<float> zonesPoints;
    int zonesPointsNum = stream.readInt();
    if ((zonesPointsNum < 0) || (zonesPointsNum > 1e3)) {
        return;
    }
    for (int i = 0; i < zonesPointsNum; ++i) {
        zonesPoints.insert(stream.readFloat());
    }
    std::vector<bool> zonesOnOff;
    int zonesOnOffNum = stream.readInt();
    if (zonesOnOffNum != zonesPointsNum + 1) {
        return;
    }
    zonesOnOff.resize(zonesOnOffNum);
    for (int i = 0; i < zonesOnOffNum; ++i) {
        zonesOnOff[i] = stream.readBool();
    }
    params.zones = Zones(static_cast<float>(num_bars), zonesPoints, zonesOnOff);

    // Read intellectual
    params.findPartialsFFTSize.store(stream.readInt());
    params.findPartialsdBThreshold.store(stream.readFloat());
    params.findPartialsStrat.store(static_cast<PartialsFindPosStrat>(stream.readInt()));
    int numTonesPartials = stream.readInt();
    DBG("Read numTonesPartials " << numTonesPartials);
    std::map<int, partialsVec> tonesPartials;
    for (int i = 0; i < numTonesPartials; ++i) {
        int totalCents = stream.readInt();
        int numPartials = stream.readInt();
        DBG("Read numPartials " << numPartials);
        partialsVec partials(numPartials);
        for (int j = 0; j < numPartials; ++j) {
            float freq = stream.readFloat();
            float amp = stream.readFloat();
            partials[j] = {freq, amp};
        }
        tonesPartials[totalCents] = partials;
    }
    params.set_tonesPartials(tonesPartials);
    params.roughCompactFrac = stream.readFloat();
    params.dissonancePow = stream.readFloat();
    // Pitch memory
    params.pitchMemoryTVvalForZeroHV = stream.readFloat();
    params.pitchMemoryTVaddInfluence = stream.readFloat();
    params.pitchMemoryTVminNonzero = stream.readFloat();

    // Read theme type
    params.themeType = static_cast<Theme::ThemeType>(stream.readInt());
    params.theme.setTheme(params.themeType);

    // Read notes
    {
        std::scoped_lock lock(notesMutex);
        notes.clear();
        int numNotes = stream.readInt();
        if ((numNotes < 0) || (numNotes > 1e6)) {
            return;
        }
        notes.reserve(numNotes);
        for (int i = 0; i < numNotes; ++i) {
            Note note;
            note.octave = stream.readInt();
            note.cents = stream.readInt();
            note.time = stream.readFloat();
            note.isSelected = stream.readBool();
            note.duration = stream.readFloat();
            note.velocity = stream.readFloat();
            note.bend = stream.readInt();
            notes.push_back(note);
        }
    }

    // Read other things
    if (!stream.isExhausted()) {
        params.pitchMemoryShowOnlyHarmonicity = stream.readBool();
    }
    if (!stream.isExhausted() && version > -3) {
        stream.readBool();
    }
    if (!stream.isExhausted()) {
        int desiredChannelIndex = stream.readInt();
        if ((params.getTuningType() == Parameters::TuningType::MTS_ESP) &&
            (desiredChannelIndex != params.channelIndex) && (desiredChannelIndex != -1)) {
            pluginInstanceManager->changeChannelIndex(desiredChannelIndex);
            params.channelIndex = pluginInstanceManager->getChannelIndex();
        }
    }

    // Read ratios marks
    if (!stream.isExhausted()) {
        params.ratiosMarks.clear();
        int numRatiosMarks = stream.readInt();
        if ((numRatiosMarks >= 0) && (numRatiosMarks < 1e6)) {
            params.ratiosMarks.reserve(numRatiosMarks);
            if (version <= -1) {
                for (int i = 0; i < numRatiosMarks; ++i) {
                    int lktc = stream.readInt();
                    int hktc = stream.readInt();
                    float t = stream.readFloat();
                    int lni = stream.readInt();
                    int hni = stream.readInt();
                    RatioMark ratioMark(lktc, hktc, t, &params, lni, hni);
                    params.ratiosMarks.push_back(ratioMark);
                }
            } else {
                for (int i = 0; i < numRatiosMarks; ++i) {
                    int lktc = stream.readInt();
                    int hktc = stream.readInt();
                    float t = stream.readFloat();
                    RatioMark ratioMark(lktc, hktc, t, &params);
                    params.ratiosMarks.push_back(ratioMark);
                }
            }
            // Other ratios marks related things
            params.autoCorrectRatiosMarks = stream.readBool();
            params.maxDenRatiosMarks = stream.readInt();
            params.goodEnoughErrorRatiosMarks = stream.readInt();
        }
    }
    params.stateHistory.push(State(num_bars, notes, params.ratiosMarks));

    // Read vocal to melody params
    if (!stream.isExhausted()) {
        params.vocalToMelodyGenCurve = stream.readBool();
        params.vocalToMelodyGenNotes = stream.readBool();
        params.vocalToMelodyMinNoteDuration = stream.readFloat();
        params.vocalToMelodyDCents = stream.readInt();
        params.vocalToMelodyKeySnap = stream.readBool();
        params.vocalToMelodyMakeBends = stream.readBool();
        if (version > -3) {
            stream.readFloat();
        }
    }

    if (!stream.isExhausted()) {
        params.maxChordDtimeClockDiagram = stream.readFloat();
    }

    // Read some state params
    if (!stream.isExhausted()) {
        params.lastDuration = stream.readFloat();
        params.lastVelocity = stream.readFloat();
        params.lastViewPos.setX(stream.readInt());
        params.lastViewPos.setY(stream.readInt());
        params.octave_height_px = stream.readFloat();
        params.bar_width_px = stream.readFloat();
        params.showGhostNotesKeys = stream.readBool();
        int numChInd = stream.readInt();
        params.ghostNotesChannels.clear();
        for (int i = 0; i < numChInd; ++i) {
            params.ghostNotesChannels.insert(stream.readInt());
        }
    }

    if (!stream.isExhausted()) {
        params.constNoteRectHeight = stream.readBool();
    }

    // UPDATE NOTES
    if (!stream.isExhausted()) {
        params.resetPitchBendOnNoteOff = stream.readBool();
        int desiredChannelIndex = stream.readInt();
        if ((params.getTuningType() == Parameters::TuningType::MPE) &&
            (desiredChannelIndex != params.channelIndex) && (desiredChannelIndex != -1)) {
            notesSharingMPE->changeInstanceId(desiredChannelIndex);
            params.channelIndex = notesSharingMPE->getInstanceId();
        }
        int numGhostInstIds = stream.readInt();
        params.ghostNotesInstIds.clear();
        for (int i = 0; i < numGhostInstIds; ++i) {
            params.ghostNotesInstIds.insert(stream.readInt());
        }
    }

    prepareNotes();
    if (params.getTuningType() == Parameters::TuningType::MPE) {
        notesSharingMPE->updateNotes(notes);
    } else if (params.getTuningType() == Parameters::TuningType::MTS_ESP) {
        pluginInstanceManager->updateNotes(notes);
    }
}

void AudioPluginAudioProcessor::updateNotes(const std::vector<Note> &new_notes) {
    // suspendProcessing(true);
    {
        std::scoped_lock lock(notesMutex);
        notes = new_notes;
    }
    prepareNotes();
    auditionChanged = true;
    // suspendProcessing(false);
    if (params.getTuningType() == Parameters::TuningType::MPE) {
        notesSharingMPE->updateNotes(notes);
    } else if (params.getTuningType() == Parameters::MTS_ESP) {
        pluginInstanceManager->updateNotes(notes);
    }
}

void AudioPluginAudioProcessor::rePrepareNotes() {
    // suspendProcessing(true);
    prepareNotes();
    // suspendProcessing(false);
}

void AudioPluginAudioProcessor::setManuallyPlayedNotes(
    const std::map<int, float> newManuallyPlayedNotes) {
    {
        std::scoped_lock lock(manPlNotesMutex);
        manuallyPlayedNotes = newManuallyPlayedNotes;
    }
    if (params.getTuningType() == Parameters::TuningType::MTS_ESP) {
        rePrepareNotes();
    }
}

const std::vector<Note> &AudioPluginAudioProcessor::getNotes() { return notes; }

std::vector<Note> AudioPluginAudioProcessor::getOtherInstancesNotes() {
    if (params.getTuningType() == Parameters::TuningType::MPE) {
        return notesSharingMPE->getInstancesNotes(params.ghostNotesInstIds);
    } else if (params.getTuningType() == Parameters::TuningType::MTS_ESP) {
        return pluginInstanceManager->getChannelsNotes(params.ghostNotesChannels);
    }
    return {};
}

float AudioPluginAudioProcessor::getPlayHeadTime() { return static_cast<float>(playHeadTime); }

double AudioPluginAudioProcessor::getNoteFreq(const Note &note) {
    float dBend = 0.0f;
    if ((note.bend != 0) && (note.time < playHeadTime) &&
        (playHeadTime <= note.time + note.duration))
        dBend = note.bend * (playHeadTime - note.time) / note.duration;
    double dCents = (note.octave * 1200 + note.cents + dBend) - (4 * 1200 + 900);
    return params.A4Freq.load() * pow(2.0, dCents / 1200.0);
}

double AudioPluginAudioProcessor::getNoteFreqAtTime(const Note &note, double time) {
    double dBend = 0.0f;
    if ((note.bend != 0) && (note.time < time) && (time <= note.time + note.duration))
        dBend = note.bend * (time - note.time) / note.duration;
    double dCents = (note.octave * 1200 + note.cents + dBend) - (4 * 1200 + 900);
    return params.A4Freq.load() * std::pow(2.0, dCents / 1200.0);
}

double AudioPluginAudioProcessor::getFreqFromTotalCents(float totalCents) {
    double dCents = totalCents - (4 * 1200 + 900);
    return params.A4Freq.load() * pow(2.0, dCents / 1200.0);
}

int AudioPluginAudioProcessor::getTotalCentsFromFreq(double freq) {
    return juce::roundToInt(1200 * log2(freq / params.A4Freq.load()) + (4 * 1200 + 900));
}

void AudioPluginAudioProcessor::prepareNotes() {
    std::scoped_lock lock(prepareNotesMutex);
    pitchesOverflow = false;
    editorKnowsAboutOverflow = false;

    if (params.getTuningType() == Parameters::TuningType::MPE) {
        return;
    }

    // Notes from piano roll
    // some cleaning
    notesIndexes.clear();
    notesIndexes.resize(notes.size());
    int numUsedFreqs = static_cast<int>(currPlayedNotesIndexes.size());
    for (int i = 0; i < 128; ++i) {
        if (!currPlayedNotesIndexes.contains(i)) {
            freqs[i] = noFreq;
        }
    }

    // set midi notes (indexes) for notes from piano roll
    int noteIndNew = 0;
    {
        std::scoped_lock lock(notesMutex);
        for (int i = 0; i < notes.size(); ++i) {
            const Note &note = notes[i];
            double noteFreq = getNoteFreq(note);
            int noteInd = findFreqInd(noteFreq);
            if (noteInd == -1) {
                if (numUsedFreqs >= 128) {
                    pitchesOverflow = true;
                    return;
                }
                while (freqs[noteIndNew] != noFreq)
                    noteIndNew++;
                noteInd = noteIndNew;
                freqs[noteInd] = noteFreq;
                numUsedFreqs++;
            }
            notesIndexes[i] = noteInd;
        }
    }

    // Manually played notes
    // make notes from manuallyPlayedNotes, that are not in piano roll, relevant
    {
        std::scoped_lock lock(manPlNotesMutex);
        for (const auto &[totalCents, _] : manuallyPlayedNotes) {
            double noteFreq = getFreqFromTotalCents(totalCents);
            int noteInd = findFreqInd(noteFreq);
            if (noteInd == -1) {
                manPlNotesTotCentsHistory.add(noteFreq);
            }
        }
    }
    // add to freqs[128] mostly relevant manually played notes that are not in freqs[128] array
    std::vector<double> manPlNotesTotCentsHistoryLast =
        manPlNotesTotCentsHistory.getLast(juce::jmax(128 - numUsedFreqs, 0));
    for (int i = 0; i < manPlNotesTotCentsHistoryLast.size(); ++i) {
        while (freqs[noteIndNew] != noFreq)
            noteIndNew++;
        if (noteIndNew >= 128) {
            pitchesOverflow = true;
            return;
        }
        freqs[noteIndNew] = manPlNotesTotCentsHistoryLast[i];
        numUsedFreqs++;
    }

    if (!params.findPartialsMode.load())
        pluginInstanceManager->updateFreqs(freqs);
}

std::tuple<float, int, int> AudioPluginAudioProcessor::getBpmNumDenom() {
    return std::make_tuple(static_cast<float>(bpm.load()), numerator.load(), denominator.load());
}

bool AudioPluginAudioProcessor::thereIsPitchOverflow() {
    if (pitchesOverflow) {
        if (editorKnowsAboutOverflow) {
            return false;
        } else {
            editorKnowsAboutOverflow = true;
            return true;
        }
    }
    return false;
}
} // namespace audio_plugin

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new audio_plugin::AudioPluginAudioProcessor();
}
