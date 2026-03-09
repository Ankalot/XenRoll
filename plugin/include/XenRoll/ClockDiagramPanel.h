#pragma once

#include "Note.h"
#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

class ClockDiagramPanel : public juce::Component {
  public:
    ClockDiagramPanel(Parameters *params, AudioPluginAudioProcessorEditor *editor,
                      const std::vector<Note> &notes);

    void setTime(float newTime) {
        time = newTime;
        refresh();
    }

    void refresh();

    void paint(juce::Graphics &g) override;

  private:
    Parameters *params;
    AudioPluginAudioProcessorEditor *editor;
    const std::vector<Note> &notes;
    float time = 0.0f;

    std::vector<std::set<int>> chordsAndNotes;
    std::set<int> allCents;

    const int padding = 36;
    const int radius = 100;
    const int labelIndent = 18;
    const float opacity = 0.8f;
    const int tickLength = 10;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClockDiagramPanel)
};
} // namespace audio_plugin