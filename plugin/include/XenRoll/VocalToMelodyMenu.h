#pragma once

#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

/**
 * @brief Small menu for changing the settings for vocal to melody mode
 */
class VocalToMelodyMenu : public juce::Component {
  public:
    VocalToMelodyMenu(Parameters *params, AudioPluginAudioProcessorEditor *editor);

    ~VocalToMelodyMenu() override {};

    void paint(juce::Graphics &g) override {
        g.fillAll(Theme::darker);
        g.setColour(Theme::darkest);
        g.drawRect(getLocalBounds(), Theme::wider);
        // Draw separators
        g.drawLine(0, vertPadding + rowHeight + buttonHeight + 2 * rowSkip, width,
                   vertPadding + rowHeight + buttonHeight + 2 * rowSkip, Theme::wider);
        g.drawLine(0, vertPadding + 8 * rowHeight + buttonHeight + 8 * rowSkip, width,
                   vertPadding + 8 * rowHeight + buttonHeight + 8 * rowSkip, Theme::wider);
    }

  private:
    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;

    std::unique_ptr<juce::Label> vocalToMelodyGenCurveLabel, vocalToMelodyGenNotesLabel,
        vocalToMelodyMinNoteDurationLabel, vocalToMelodyDcentsLabel, vocalToMelodyKeySnapLabel,
        vocalToMelodyMakeBendsLabel, micGain_dBLabel;
    std::unique_ptr<juce::ComboBox> vocalToMelodyMinNoteDurationCombo;
    std::unique_ptr<juce::Slider> vocalToMelodyDcentsSlider, micGain_dBSlider;
    std::unique_ptr<juce::ToggleButton> vocalToMelodyGenCurveCheckbox,
        vocalToMelodyGenNotesCheckbox, vocalToMelodyKeySnapCheckbox, vocalToMelodyMakeBendsCheckbox;
    std::unique_ptr<juce::Button> vocalToMelodyDeleteCurveButton;

    const int width = 250;
    const int rowSkip = 10;
    const int horPadding = 15;
    const int vertPadding = 10;
    const int rowHeight = 24;
    const int buttonHeight = 32;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalToMelodyMenu)
};
} // namespace audio_plugin