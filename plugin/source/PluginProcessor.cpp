#include "XenRoll/PluginProcessor.h"
#include "XenRoll/PluginEditor.h"

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

    // INSTANCES SYNC
    pluginInstanceManager = std::make_unique<PluginInstanceManager>();
    isActive = pluginInstanceManager->getIsActive();
    params.channelIndex = pluginInstanceManager->getChannelIndex();

    // PARTIALS FINDING
    threadPool = std::make_unique<juce::ThreadPool>(1);
    partialsFinder = std::make_shared<PartialsFinder>();
    wasPianoRoll = !params.findPartialsMode.load();
    for (int i = 0; i < params.num_octaves * 12; ++i) {
        freqs12EDO[i] = getFreqFromTotalCents(i * 100.0f);
    }
    partialsFinderBuffer = std::make_unique<AccumulatingBuffer>();

    std::fill(std::begin(beforeBendTotalCents), std::end(beforeBendTotalCents), -1);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

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
    juce::ignoreUnused(sampleRate, samplesPerBlock);
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
        if (freqs[i] == freq) {
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

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                             juce::MidiBuffer &midiMessages) {
    if (!isActive)
        return;

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
            pluginInstanceManager->updateFreqs(freqs12EDO);
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
            rePrepareNotes();
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
            double beatsPerBar =
                static_cast<double>(timeSig.numerator) / (timeSig.denominator / 4.0);
            double ppqPosition = positionInfo->getPpqPosition().orFallback(0.0);
            playHeadTime = ppqPosition / beatsPerBar;

            double bpm = positionInfo->getBpm().orFallback(120.0);
            double secondsPerBeat = 60.0 / bpm;
            double samplesPerBeat = secondsPerBeat * sampleRate;
            double beatsInBlock = numSamples / samplesPerBeat;
            double barsInBlock = beatsInBlock / beatsPerBar;
            bool isPlaying = positionInfo->getIsPlaying();

            // Start playing manually pressed notes
            if (!startedPlayingManuallyPressedNotes) {
                int i = 0;
                for (const int totalCents : manuallyPlayedNotesTotalCents) {
                    if (manuallyPlayedNotesAreNew[i]) {
                        int freqInd = manuallyPlayedNotesIndexes[i];
                        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(
                            params.channelIndex + 1, freqInd, 100.0f / 127);
                        midiMessages.addEvent(noteOn, 0);
                        currPlayedNotesIndexes.insert(freqInd);
                    }
                    i++;
                }
                startedPlayingManuallyPressedNotes = true;
            }

            // Play notes from piano roll
            if (isPlaying) {
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
                            int(floor(numSamples * (note.time + note.duration - playHeadTime) /
                                      barsInBlock)));
                        currPlayedNotesTotalCents.erase(note.octave * 1200 + note.cents);
                    }
                }
                for (int i = 0; i < notes.size(); ++i) {
                    const Note &note = notes[i];
                    // Note on
                    if ((note.time >=
                         playHeadTime) && /*!currPlayedNotesTotalCents.contains(note.octave*1200+note.cents)*/
                        (note.time < playHeadTime + barsInBlock)) {
                        const int noteInd = notesIndexes[i];
                        // if was bending - update frequency
                        if (beforeBendTotalCents[noteInd] != -1) {
                            freqs[noteInd] = getFreqFromTotalCents(note.octave * 1200 + note.cents);
                            beforeBendTotalCents[noteInd] = -1;
                            pluginInstanceManager->updateFreqs(freqs);
                        }
                        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(
                            params.channelIndex + 1, noteInd, note.velocity);
                        midiMessages.addEvent(
                            noteOn,
                            int(ceil(numSamples * (note.time - playHeadTime) / barsInBlock)));
                        currPlayedNotesTotalCents.insert(note.octave * 1200 + note.cents);
                        currPlayedNotesIndexes.insert(noteInd);
                        if (note.bend != 0) {
                            beforeBendTotalCents[noteInd] = note.octave * 1200 + note.cents;
                        }
                    }
                    // Note bend
                    if ((note.bend != 0) && (note.time < playHeadTime) &&
                        (playHeadTime <= note.time + note.duration)) {
                        freqs[notesIndexes[i]] = getNoteFreq(note);
                        pluginInstanceManager->updateFreqs(freqs);
                    }
                }
            } else {
                currPlayedNotesTotalCents.clear();
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
            for (auto it = currPlayedNotesIndexes.begin(); it != currPlayedNotesIndexes.end();) {
                int ind = *it;
                bool thereStillExistsThisNote = false;
                if (isPlaying) {
                    for (int i = 0; i < notes.size(); ++i) {
                        const Note &note = notes[i];
                        if ((notesIndexes[i] == ind) && (note.time < playHeadTime + barsInBlock) &&
                            (note.time + note.duration >= playHeadTime + barsInBlock) &&
                            (getNoteFreq(note) == freqs[ind])) {
                            thereStillExistsThisNote = true;
                            break;
                        }
                    }
                }
                if (!thereStillExistsThisNote) {
                    for (int i = 0; i < manuallyPlayedNotesIndexes.size(); ++i) {
                        if (manuallyPlayedNotesIndexes[i] == ind) {
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
                    midiMessages.addEvent(noteOff, 1);
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

    buffer.clear();
}

bool AudioPluginAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor() {
    return new AudioPluginAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    juce::MemoryOutputStream stream(destData, false);

    // Write simple params
    stream.writeInt(params.editorWidth);
    stream.writeInt(params.editorHeight);
    stream.writeInt(params.get_num_bars());
    stream.writeInt(params.num_beats);
    stream.writeInt(params.num_subdivs);
    stream.writeInt(params.start_octave);
    stream.writeDouble(params.A4Freq.load());
    stream.writeFloat(params.noteRectHeightCoef);

    // Write zones
    auto &zonesPoints = params.zones.getZonesPoints();
    stream.writeInt(static_cast<int>(zonesPoints.size()));
    for (const auto &zp : zonesPoints) {
        stream.writeFloat(zp);
    }
    auto &zonesOnOff = params.zones.getZonesOnOff();
    stream.writeInt(static_cast<int>(zonesOnOff.size()));
    for (const auto &zof : zonesOnOff) {
        stream.writeBool(zof);
    }

    // Write intellectual
    stream.writeInt(params.findPartialsFFTSize.load());
    stream.writeFloat(params.findPartialsdBThreshold.load());
    stream.writeInt(static_cast<int>(params.findPartialsStrat.load()));
    const auto tonePartials = params.get_tonesPartials();
    stream.writeInt((int)tonePartials.size());
    DBG("Write tonePartials.size() " << tonePartials.size());
    for (const auto &[totalCents, partials] : tonePartials) {
        stream.writeInt(totalCents);
        stream.writeInt((int)partials.size());
        DBG("Write partials.size() " << partials.size());
        for (const auto &[freq, amp] : partials) {
            stream.writeFloat(freq);
            stream.writeFloat(amp);
        }
    }
    stream.writeFloat(params.roughCompactFrac);
    stream.writeFloat(params.dissonancePow);
    // Pitch memory
    stream.writeFloat(params.pitchMemoryTVvalForZeroHV);
    stream.writeFloat(params.pitchMemoryTVaddInfluence);
    stream.writeFloat(params.pitchMemoryTVminNonzero);

    // Write theme type
    stream.writeInt(static_cast<int>(params.themeType));

    // Write notes
    stream.writeInt(static_cast<int>(notes.size()));
    for (const auto &note : notes) {
        stream.writeInt(note.octave);
        stream.writeInt(note.cents);
        stream.writeFloat(note.time);
        stream.writeBool(note.isSelected);
        stream.writeFloat(note.duration);
        stream.writeFloat(note.velocity);
        stream.writeInt(note.bend);
    }

    // Write other things
    stream.writeBool(params.pitchMemoryShowOnlyHarmonicity);
    stream.writeBool(params.playDraggedNotes);
    stream.writeInt(params.channelIndex);

    // Write ratios marks
    stream.writeInt(static_cast<int>(params.ratiosMarks.size()));
    for (const auto &ratioMark : params.ratiosMarks) {
        stream.writeInt(ratioMark.getLowerKeyTotalCents());
        stream.writeInt(ratioMark.getHigherKeyTotalCents());
        stream.writeFloat(ratioMark.time);
    }
    // Other ratios marks related things
    stream.writeBool(params.autoCorrectRatiosMarks);
    stream.writeInt(params.maxDenRatiosMarks);
    stream.writeInt(params.goodEnoughErrorRatiosMarks);
}

void AudioPluginAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    if (data == nullptr || sizeInBytes <= 0) {
        notes.clear();
        return;
    }
    juce::MemoryInputStream stream(data, sizeInBytes, false);

    // Read simple params
    params.editorWidth = stream.readInt();
    params.editorHeight = stream.readInt();
    int num_bars = stream.readInt();
    params.set_num_bars(num_bars);
    params.num_beats = stream.readInt();
    params.num_subdivs = stream.readInt();
    params.start_octave = stream.readInt();
    params.A4Freq.store(stream.readDouble());
    params.noteRectHeightCoef = stream.readFloat();

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
    params.zones = Zones(float(num_bars), zonesPoints, zonesOnOff);

    // Read intellectual
    params.findPartialsFFTSize.store(stream.readInt());
    params.findPartialsdBThreshold.store(stream.readFloat());
    params.findPartialsStrat.store(static_cast<PartialsFinder::PosFindStrat>(stream.readInt()));
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

    // Read notes
    notes.clear();
    int numNotes = stream.readInt();
    if ((numNotes <= 0) || (numNotes > 1e6)) {
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

    // Read other things
    if (!stream.isExhausted()) {
        params.pitchMemoryShowOnlyHarmonicity = stream.readBool();
    }
    if (!stream.isExhausted()) {
        params.playDraggedNotes = stream.readBool();
    }
    if (!stream.isExhausted()) {
        int desiredChannelIndex = stream.readInt();
        if ((desiredChannelIndex != params.channelIndex) && (desiredChannelIndex != -1)) {
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
            for (int i = 0; i < numRatiosMarks; ++i) {
                int lktc = stream.readInt();
                int hktc = stream.readInt();
                float t = stream.readFloat();
                RatioMark ratioMark(lktc, hktc, t, &params);
                params.ratiosMarks.push_back(ratioMark);
            }
            // Other ratios marks related things
            params.autoCorrectRatiosMarks = stream.readBool();
            params.maxDenRatiosMarks = stream.readInt();
            params.goodEnoughErrorRatiosMarks = stream.readInt();
        }
    }

    // UPDATE NOTES
    prepareNotes();
    pluginInstanceManager->updateNotes(notes);
}

void AudioPluginAudioProcessor::updateNotes(const std::vector<Note> &new_notes) {
    suspendProcessing(true);
    notes = new_notes;
    prepareNotes();
    suspendProcessing(false);
    pluginInstanceManager->updateNotes(notes);
}

void AudioPluginAudioProcessor::rePrepareNotes() {
    suspendProcessing(true);
    prepareNotes();
    suspendProcessing(false);
}

void AudioPluginAudioProcessor::setManuallyPlayedNotesTotalCents(
    const std::set<int> newManuallyPlayedNotesTotalCents) {
    suspendProcessing(true);
    std::scoped_lock lock(manPlNotesTotCentsMutex);
    manuallyPlayedNotesAreNew.clear();
    manuallyPlayedNotesAreNew.resize(newManuallyPlayedNotesTotalCents.size());
    int i = 0;
    for (int newTotalCents : newManuallyPlayedNotesTotalCents) {
        manuallyPlayedNotesAreNew[i] = !manuallyPlayedNotesTotalCents.contains(newTotalCents);
        i++;
    }
    manuallyPlayedNotesTotalCents = newManuallyPlayedNotesTotalCents;
    startedPlayingManuallyPressedNotes = false;
    prepareNotes();
    pluginInstanceManager->waitChannelUpdate();
    suspendProcessing(false);
}

const std::vector<Note> &AudioPluginAudioProcessor::getNotes() { return notes; }

std::vector<Note> AudioPluginAudioProcessor::getOtherInstancesNotes() {
    return pluginInstanceManager->getChannelsNotes(params.ghostNotesChannels);
}

float AudioPluginAudioProcessor::getPlayHeadTime() { return float(playHeadTime); }

double AudioPluginAudioProcessor::getNoteFreq(const Note &note) {
    float dBend = 0.0f;
    if ((note.bend != 0) && (note.time < playHeadTime) &&
        (playHeadTime <= note.time + note.duration))
        dBend = note.bend * (playHeadTime - note.time) / note.duration;
    float dCents = (note.octave * 1200 + note.cents + dBend) - (4 * 1200 + 900);
    return params.A4Freq.load() * pow(2.0, dCents / 1200.0);
}

double AudioPluginAudioProcessor::getFreqFromTotalCents(float totalCents) {
    float dCents = totalCents - (4 * 1200 + 900);
    return params.A4Freq.load() * pow(2.0, dCents / 1200.0);
}

int AudioPluginAudioProcessor::getTotalCentsFromFreq(double freq) {
    return int(round(1200 * log2(freq / params.A4Freq.load()) + (4 * 1200 + 900)));
}

void AudioPluginAudioProcessor::prepareNotes() {
    // Notes from piano roll
    // some cleaning
    notesIndexes.clear();
    notesIndexes.resize(notes.size());
    int numUsedFreqs = int(currPlayedNotesIndexes.size());
    for (int i = 0; i < 128; ++i) {
        if (!currPlayedNotesIndexes.contains(i)) {
            freqs[i] = 0.0;
        }
    }
    // set midi notes (indexes) for notes from piano roll
    int noteIndNew = 0;
    for (int i = 0; i < notes.size(); ++i) {
        const Note &note = notes[i];
        double noteFreq = getNoteFreq(note);
        int noteInd = findFreqInd(noteFreq);
        if (noteInd == -1) {
            if (numUsedFreqs == 128) {
                pitchesOverflow = true;
                return;
            }
            while (freqs[noteIndNew] != 0.0)
                noteIndNew++;
            noteInd = noteIndNew;
            freqs[noteInd] = noteFreq;
            numUsedFreqs++;
        }
        notesIndexes[i] = noteInd;
    }

    // Manually played notes
    // make notes from manuallyPlayedNotesTotalCents, that are not in piano roll, relevant
    for (const int totalCents : manuallyPlayedNotesTotalCents) {
        double noteFreq = getFreqFromTotalCents(totalCents);
        int noteInd = findFreqInd(noteFreq);
        if (noteInd == -1) {
            manPlNotesTotCentsHistory.add(noteFreq);
        }
    }
    // add to freqs[128] mostly relevant manually pressed notes that are not in freqs[128] array
    std::vector<double> manPlNotesTotCentsHistoryLast =
        manPlNotesTotCentsHistory.getLast(128 - numUsedFreqs);
    for (int i = 0; i < manPlNotesTotCentsHistoryLast.size(); ++i) {
        while (freqs[noteIndNew] != 0.0)
            noteIndNew++;
        freqs[noteIndNew] = manPlNotesTotCentsHistoryLast[i];
        numUsedFreqs++;
    }
    manuallyPlayedNotesIndexes.clear();
    manuallyPlayedNotesIndexes.resize(manuallyPlayedNotesTotalCents.size());
    // set midi notes (indexes) for manually pressed notes
    int i = 0;
    for (const int totalCents : manuallyPlayedNotesTotalCents) {
        double noteFreq = getFreqFromTotalCents(totalCents);
        int noteInd = findFreqInd(noteFreq);
        if (noteInd == -1) {
            pitchesOverflow = true;
            return;
        }
        manuallyPlayedNotesIndexes[i] = noteInd;
        i++;
    }
    if (!params.findPartialsMode.load())
        pluginInstanceManager->updateFreqs(freqs);
    pitchesOverflow = false;
    editorKnowsAboutOverflow = false;
}

std::tuple<float, int, int> AudioPluginAudioProcessor::getBpmNumDenom() {
    auto playhead = getPlayHead();
    if (playhead != nullptr) {
        auto posInfo = playhead->getPosition();
        if (posInfo.hasValue()) {
            auto timeSig =
                posInfo->getTimeSignature().orFallback(juce::AudioPlayHead::TimeSignature{4, 4});
            auto bpm = posInfo->getBpm().orFallback(120.0);
            return std::make_tuple(float(bpm), timeSig.numerator, timeSig.denominator);
        }
    }
    return std::make_tuple(120.0, 4, 4);
}

std::set<int> AudioPluginAudioProcessor::getAllCurrPlayedNotesTotalCents() {
    suspendProcessing(true);
    std::set<int> result(currPlayedNotesTotalCents);
    std::scoped_lock lock(manPlNotesTotCentsMutex);
    result.insert(manuallyPlayedNotesTotalCents.begin(), manuallyPlayedNotesTotalCents.end());
    suspendProcessing(false);

    return result;
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
