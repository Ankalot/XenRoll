#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
/**
 * @brief Thread-safe buffer for accumulating audio samples over time
 * @note Automatically mixes multi-channel input to mono
 */
class AccumulatingBuffer {
  public:
    /**
     * @brief Construct an AccumulatingBuffer with default size
     */
    AccumulatingBuffer() {
        buffer.setSize(1, 1024);
        buffer.clear();
    }

    /**
     * @brief Add samples to the buffer
     * @param input Audio buffer to add (multi-channel input is mixed to mono)
     * @note Thread-safe. Automatically resizes buffer if needed.
     */
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

    /**
     * @brief Extract all accumulated samples and clear the buffer
     * @return Audio buffer containing all accumulated samples (mono)
     * @note Thread-safe
     */
    juce::AudioBuffer<float> extractAndClear() {
        const juce::ScopedLock lock(mutex); // Thread-safe

        // Create a copy of the current data
        juce::AudioBuffer<float> result(1, writePos);
        result.copyFrom(0, 0, buffer, 0, 0, writePos);

        clear();

        return result;
    }

    /**
     * @brief Clear the buffer
     * @note Thread-safe
     */
    void clear() {
        const juce::ScopedLock lock(mutex); // Thread-safe
        buffer.clear();
        writePos = 0;
    }

  private:
    juce::AudioBuffer<float> buffer;
    int writePos = 0;
    juce::CriticalSection mutex;
};
} // namespace audio_plugin