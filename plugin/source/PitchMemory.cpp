#include "XenRoll/PitchMemory.h"

namespace audio_plugin {
PitchMemory::PitchMemory(std::shared_ptr<DissonanceMeter> dissonanceMeter, float TV_val_for_zero_HV,
                         float TV_add_influence, float TV_min_nonzero, int num_octaves)
    : dissonanceMeter(dissonanceMeter), TV_val_for_zero_HV(TV_val_for_zero_HV),
      TV_add_influence(TV_add_influence), TV_min_nonzero(TV_min_nonzero), num_octaves(num_octaves) {
}

void PitchMemory::set_TV_val_for_zero_HV(float new_TV_val_for_zero_HV) {
    TV_val_for_zero_HV = new_TV_val_for_zero_HV;
}

void PitchMemory::set_TV_add_influence(float new_TV_add_influence) {
    TV_add_influence = new_TV_add_influence;
}

void PitchMemory::set_TV_min_nonzero(float new_TV_min_nonzero) {
    TV_min_nonzero = new_TV_min_nonzero;
}

std::optional<PitchMemoryResults> PitchMemory::findPitchTraces(const std::vector<Note> &notes,
                                                               const std::atomic<bool> &terminate) {
    PitchTraces pitchTraces;
    std::vector<float> notesHarmonicity(notes.size());

    // 1. Preparation
    // 1.1 Sort notes by time. if time is equal sort by pitch.
    std::vector<int> notesIndexes(notes.size());
    std::iota(notesIndexes.begin(), notesIndexes.end(), 0);
    std::sort(notesIndexes.begin(), notesIndexes.end(), [&notes](size_t i1, size_t i2) {
        if (notes[i1].time == notes[i2].time) {
            return notes[i1].octave * 1200 + notes[i1].cents >
                   notes[i2].octave * 1200 + notes[i2].cents;
        } else {
            return notes[i1].time < notes[i2].time;
        }
    });

    std::vector<Note> sortedNotes;
    sortedNotes.reserve(notes.size());
    for (auto i : notesIndexes) {
        sortedNotes.push_back(notes[i]);
    }
    // 1.2 All present pitches
    std::set<int> pitchesTotalCentsSet;
    for (const Note &note : sortedNotes) {
        pitchesTotalCentsSet.insert(note.octave * 1200 + note.cents);
    }
    std::vector<int> pitchesTotalCents(pitchesTotalCentsSet.begin(), pitchesTotalCentsSet.end());
    const int numPitches = pitchesTotalCents.size();
    std::map<int, int> totalCentsToIndex;
    for (int i = 0; i < numPitches; ++i) {
        totalCentsToIndex[pitchesTotalCents[i]] = i;
    }
    pitchTraces.first = pitchesTotalCents;

    // 2. First note
    int numNotes = sortedNotes.size();
    if (numNotes == 0) {
        return std::make_pair(pitchTraces, notesHarmonicity);
    }
    const Note &firstNote = sortedNotes[0];
    std::vector<float> tracesValues(numPitches, 0.0f);
    tracesValues[totalCentsToIndex[firstNote.octave * 1200 + firstNote.cents]] = 1.0f;
    pitchTraces.second[firstNote.time] = tracesValues;
    notesHarmonicity[notesIndexes[0]] = 1.0f;

    // 3. Other notes
    std::map<std::pair<int, int>, float> cashedDissonances;
    std::vector<float> dissonanceValues(numPitches, 0.0f);
    for (int noteInd = 1; noteInd < numNotes; ++noteInd) {
        if (terminate) {
            return std::nullopt;
        }
        const Note &note = sortedNotes[noteInd];
        const int totalCents = note.octave * 1200 + note.cents;
        const int totalCentsInd = totalCentsToIndex[totalCents];

        // 3.1 Find DVs between note and all other traces
        for (int i = 0; i < numPitches; ++i) {
            const int traceTotalCents = pitchesTotalCents[i];
            // if (traceTotalCents != totalCents) {
            int totalCents1 = std::min(totalCents, traceTotalCents);
            int totalCents2 = std::max(totalCents, traceTotalCents);
            float DV = 0;
            const auto it = cashedDissonances.find({totalCents1, totalCents2});
            if (it == cashedDissonances.end()) {
                DV = dissonanceMeter->calcDissonance(totalCents1, totalCents2);
                cashedDissonances[{totalCents1, totalCents2}] = DV;
            } else {
                DV = it->second;
            }
            dissonanceValues[i] = DV;
            //}
        }

        // 3.2 Find notes' HV
        float sum1 = 0.0f;
        float sum2 = 0.0f;
        // tracesValues[totalCentsInd] = 0.0f;
        for (int i = 0; i < numPitches; ++i) {
            sum1 += tracesValues[i] * dissonanceValues[i];
            sum2 += tracesValues[i];
        }
        float HV = 0;
        if (sum2 != 0) {
            HV = -sum1 / sum2;
        }
        notesHarmonicity[notesIndexes[noteInd]] = HV;

        // 3.3 Find notes' TV
        float TV = 0.0f;
        if (HV <= 0) {
            TV = (HV + 1) * TV_val_for_zero_HV;
        } else {
            TV = (1 - TV_val_for_zero_HV) * HV + TV_val_for_zero_HV;
        }
        if (TV < TV_min_nonzero) {
            TV = 0.0f;
        }
        tracesValues[totalCentsInd] = TV;

        // 3.4 Change all other traces' values
        for (int i = 0; i < numPitches; ++i) {
            const int traceTotalCents = pitchesTotalCents[i];
            if (traceTotalCents != totalCents) {
                tracesValues[i] = std::min(
                    1.0, tracesValues[i] * std::pow(2, std::min(1.0f, TV + TV_add_influence) * HV));
                if (tracesValues[i] < TV_min_nonzero) {
                    tracesValues[i] = 0.0f;
                }
            }
        }
        pitchTraces.second[note.time] = tracesValues;
    }

    return std::make_pair(pitchTraces, notesHarmonicity);
}

std::optional<std::map<int, float>>
PitchMemory::findKeysHarmonicity(const PitchMemoryResults &pitchMemoryResults,
                                 const std::atomic<bool> &terminate) {
    // Data from pitchMemoryResults
    const std::vector<int> &pitchesTotalCents = pitchMemoryResults.first.first;
    const int numPitches = pitchesTotalCents.size();
    if (numPitches == 0) {
        return {};
    }
    const auto &lastTVs = pitchMemoryResults.first.second.rbegin()->second;

    // Keys (in cents)
    std::set<int> keys;
    for (const int &ptc : pitchesTotalCents) {
        keys.insert(ptc % 1200);
    }

    // Octaves, the keys in which will be considered
    const int minOctave = std::max(0, pitchesTotalCents[0] / 1200 - 1);
    const int maxOctave =
        std::min(num_octaves - 1, pitchesTotalCents[pitchesTotalCents.size() - 1] / 1200 + 1);

    // Finding harmonicity of this keys
    std::map<int, float> keysHarmonicity;
    std::vector<float> dissonanceValues(numPitches);
    for (int octave = minOctave; octave <= maxOctave; ++octave) {
        for (int cents : keys) {
            if (terminate) {
                return std::nullopt;
            }

            const int totalCents = octave * 1200 + cents;

            // Find dissonance values between key and traces
            for (int i = 0; i < numPitches; ++i) {
                const int traceTotalCents = pitchesTotalCents[i];
                // if (traceTotalCents != totalCents) {
                int totalCents1 = std::min(totalCents, traceTotalCents);
                int totalCents2 = std::max(totalCents, traceTotalCents);
                float DV = dissonanceMeter->calcDissonance(totalCents1, totalCents2);
                dissonanceValues[i] = DV;
                //}
            }

            // Find harmonicity value of key
            float sum1 = 0.0f;
            float sum2 = 0.0f;
            for (int i = 0; i < numPitches; ++i) {
                // if (pitchesTotalCents[i] != totalCents) {
                sum1 += lastTVs[i] * dissonanceValues[i];
                sum2 += lastTVs[i];
                //}
            }
            float HV = 0;
            if (sum2 != 0) {
                HV = -sum1 / sum2;
            }
            keysHarmonicity[totalCents] = HV;
        }
    }

    return keysHarmonicity;
}
} // namespace audio_plugin