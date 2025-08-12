#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AccumulatingBuffer {
  public:
    AccumulatingBuffer() {
        buffer.setSize(1, 1024);
        buffer.clear();
    }

    void addSamples(const juce::AudioBuffer<float> &input) {
        const juce::ScopedLock lock(mutex); // Thread-safe

        // Resize if needed (e.g., incoming buffer is larger)
        const int newNumSamples = buffer.getNumSamples() + input.getNumSamples();
        if (newNumSamples > buffer.getNumSamples()) {
            buffer.setSize(1, newNumSamples, true); // Keep existing content
        }

        // Append input to buffer (assuming mono input or mixdown)
        for (int i = 0; i < input.getNumSamples(); ++i) {
            float sample = 0.0f;
            for (int ch = 0; ch < input.getNumChannels(); ++ch) {
                sample += input.getSample(ch, i);
            }
            buffer.addSample(0, writePos++, sample / input.getNumChannels()); // Mix to mono
        }
    }

    juce::AudioBuffer<float> extractAndClear() {
        const juce::ScopedLock lock(mutex); // Thread-safe

        // Create a copy of the current data
        juce::AudioBuffer<float> result(1, writePos);
        result.copyFrom(0, 0, buffer, 0, 0, writePos);

        clear();

        return result;
    }

    void clear() {
        buffer.clear();
        writePos = 0;
    }

  private:
    juce::AudioBuffer<float> buffer;
    int writePos = 0;
    juce::CriticalSection mutex;
};
} // namespace audio_plugin