#pragma once

#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

class SettingsPanel : public juce::Component {
  public:
    SettingsPanel(Parameters *params, AudioPluginAudioProcessorEditor *editor);

    void resized() override;
    void paint(juce::Graphics &g) override;

  private:
    std::unique_ptr<juce::Label> startingOctaveLabel;
    std::unique_ptr<juce::ComboBox> startingOctaveCombo;

    std::unique_ptr<juce::Label> a4FreqLabel;
    std::unique_ptr<juce::Slider> a4FreqSlider;

    std::unique_ptr<juce::Label> heightCoefLabel;
    std::unique_ptr<juce::Slider> heightCoefSlider;

    std::unique_ptr<juce::Label> themeTypeLabel;
    std::unique_ptr<juce::ComboBox> themeTypeCombo;

    std::unique_ptr<juce::Label> playDraggedNotesLabel;
    std::unique_ptr<juce::ToggleButton> playDraggedNotesCheckbox;

    const int padding = 15;
    const int rowHeight = 32;
    const int labelWidth = 350;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};
} // namespace audio_plugin