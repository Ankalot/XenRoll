#pragma once

#include "XenRoll/data/GlobalSettings.h"
#include "XenRoll/data/Parameters.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

class SettingsViewport : public juce::Viewport {
  public:
    SettingsViewport() {
        setScrollBarsShown(true, false);
        setViewportIgnoreDragFlag(true);
    }

    void visibilityChanged() override {
        auto *settingsPanel = getViewedComponent();
        if (settingsPanel) {
            settingsPanel->visibilityChanged();
        }
    }
};

class SettingsPanel : public juce::Component {
  public:
    SettingsPanel(Parameters &params, AudioPluginAudioProcessorEditor &editor);

    void resized() override;
    void paint(juce::Graphics &g) override;

    void visibilityChanged() override {
        if (isVisible()) {
            playDraggedNotesCheckbox->setToggleState(
                GlobalSettings::getInstance().getPlayDraggedNotes(), juce::dontSendNotification);
            chaseMIDINotesCheckbox->setToggleState(
                GlobalSettings::getInstance().getChaseMIDINotes(), juce::dontSendNotification);
            horZoomOnCursorCheckbox->setToggleState(
                GlobalSettings::getInstance().getHorZoomOnCursor(), juce::dontSendNotification);
            noteRectRoundingSlider->setValue(GlobalSettings::getInstance().getNoteRectRounding(),
                                             juce::dontSendNotification);
        }
    }

    int getRequiredHeight() const;

  private:
    Parameters &params;

    // Section headers
    std::unique_ptr<juce::Label> basicSettingsHeader;
    std::unique_ptr<juce::Label> visualSettingsHeader;
    std::unique_ptr<juce::Label> mpeSettingsHeader;

    // Basic settings
    std::unique_ptr<juce::Label> startingOctaveLabel;
    std::unique_ptr<juce::ComboBox> startingOctaveCombo;

    std::unique_ptr<juce::Label> a4FreqLabel;
    std::unique_ptr<juce::Slider> a4FreqSlider;

    std::unique_ptr<juce::Label> playDraggedNotesLabel;
    std::unique_ptr<juce::ToggleButton> playDraggedNotesCheckbox;

    std::unique_ptr<juce::Label> chaseMIDINotesLabel;
    std::unique_ptr<juce::ToggleButton> chaseMIDINotesCheckbox;

    std::unique_ptr<juce::Label> horZoomOnCursorLabel;
    std::unique_ptr<juce::ToggleButton> horZoomOnCursorCheckbox;

    // Visual settings
    std::unique_ptr<juce::Label> themeTypeLabel;
    std::unique_ptr<juce::ComboBox> themeTypeCombo;

    std::unique_ptr<juce::Label> noteRectRoundingLabel;
    std::unique_ptr<juce::Slider> noteRectRoundingSlider;

    std::unique_ptr<juce::Label> heightCoefLabel;
    std::unique_ptr<juce::Slider> heightCoefSlider;

    std::unique_ptr<juce::Label> constNoteRectHeightLabel;
    std::unique_ptr<juce::ToggleButton> constNoteRectHeightCheckbox;

    // MPE tuning mode settings
    std::unique_ptr<juce::Label> semiBendRangeLabel;
    std::unique_ptr<juce::ComboBox> semiBendRangeCombo;

    std::unique_ptr<juce::Label> resetPitchBendOnNoteOffLabel;
    std::unique_ptr<juce::ToggleButton> resetPitchBendOnNoteOffCheckbox;

    std::unique_ptr<juce::Label> channelsEconomyModeMPELabel;
    std::unique_ptr<juce::ToggleButton> channelsEconomyModeMPECheckbox;

    const int padding = 8;
    const int rowHeight = 28;
    const int headerRowHeight = 34;
    const int labelWidth = 400;
    const int sectionSpacing = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};
} // namespace audio_plugin