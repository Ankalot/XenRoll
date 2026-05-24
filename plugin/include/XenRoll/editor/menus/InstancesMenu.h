#pragma once

#include "XenRoll/data/Parameters.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

class InstancesMenu : public juce::Component {
  public:
    InstancesMenu(Parameters *params, AudioPluginAudioProcessorEditor *editor);
    ~InstancesMenu() override = default;

    void buildMenu();

    void visibilityChanged() override {
        if (isVisible()) {
            std::set<int> mbNewPossibleInds = getPossibleInds();
            if (mbNewPossibleInds != possibleInds) {
                possibleInds = mbNewPossibleInds;
                buildMenu();
            }
        }
    }

    void paint(juce::Graphics &g) override {
        g.fillAll(params->theme.darker);
        g.setColour(params->theme.darkest);
        g.drawRect(getLocalBounds().toFloat(), Theme::wider);
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(Theme::wider);
        titleLabel->setBounds(bounds.removeFromTop(titleHeight));
        viewport.setBounds(bounds.withTrimmedRight(viewportMargin)
                               .withTrimmedBottom(viewportMargin)
                               .withTrimmedLeft(viewportMargin));
    }

  private:
    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;

    int myInd;
    std::set<int> possibleInds;

    // Viewport + inner content
    juce::Viewport viewport;
    juce::Component contentComponent; // Holds all labels & checkboxes

    std::unique_ptr<juce::Label> titleLabel;
    std::vector<std::unique_ptr<juce::Label>> instancesLabels;
    std::vector<std::unique_ptr<juce::ToggleButton>> instancesCheckboxes;

    const int width = 200;
    const int height = 200;
    const int titleHeight = 32;
    const int rowHeight = 24;
    const int viewportMargin = 10;
    const int viewportHorPadding = 10;
    juce::Font font;

    std::set<int> getPossibleInds();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstancesMenu)
};
} // namespace audio_plugin