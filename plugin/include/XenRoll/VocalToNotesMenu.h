#pragma once

#include "Parameters.h"
#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

/**
 * @brief Small menu for changing the settings for vocal to notes mode
 */
class VocalToNotesMenu : public juce::Component {
  public:
    VocalToNotesMenu(Parameters *params, AudioPluginAudioProcessorEditor *editor);

    ~VocalToNotesMenu() override {};

    void paint(juce::Graphics &g) override {
        g.fillAll(Theme::darker);
        g.setColour(Theme::darkest);
        g.drawRect(getLocalBounds(), Theme::wider);
    }

  private:
    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;

    std::unique_ptr<juce::Label> vocalToNotesDcentsLabel, vocalToNotesKeySnapLabel,
        vocalToNotesMakeBendsLabel, micGain_dBLabel;
    std::unique_ptr<juce::Slider> vocalToNotesDcentsSlider, micGain_dBSlider;
    std::unique_ptr<juce::ToggleButton> vocalToNotesKeySnapCheckbox, vocalToNotesMakeBendsCheckbox;

    const int width = 250;
    const int rowSkip = 10;
    const int horPadding = 15;
    const int vertPadding = 10;
    const int rowHeight = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalToNotesMenu)
};
} // namespace audio_plugin