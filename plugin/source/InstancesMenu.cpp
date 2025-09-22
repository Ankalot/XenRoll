#include "XenRoll/InstancesMenu.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
    InstancesMenu::InstancesMenu(const int chIndex, Parameters *params, AudioPluginAudioProcessorEditor *editor)
        : chIndex(chIndex), params(params), editor(editor) {
        setAlwaysOnTop(true);
        setWantsKeyboardFocus(false);
        setVisible(false);

        // Setup title
        titleLabel = std::make_unique<juce::Label>();
        titleLabel->setColour(juce::Label::backgroundColourId, Theme::darker);
        titleLabel->setText("View notes from instances", juce::dontSendNotification);
        juce::Font currentFont = titleLabel->getFont();
        currentFont.setHeight(Theme::small_);
        titleLabel->setFont(currentFont);
        titleLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(titleLabel.get());

        // Setup labels and checkboxes
        for (int i = 0; i < 16; ++i) {
            if (i != chIndex) {
                channelsLabels[i] = std::make_unique<juce::Label>();
                channelsLabels[i]->setText("MIDI Channel " + juce::String(i + 1),
                                           juce::dontSendNotification);
                channelsLabels[i]->setColour(juce::Label::backgroundColourId, Theme::darker);
                channelsLabels[i]->setFont(currentFont);
                addAndMakeVisible(channelsLabels[i].get());

                channelsCheckboxes[i] = std::make_unique<juce::ToggleButton>();
                channelsCheckboxes[i]->setToggleState(params->ghostNotesChannels.contains(i),
                                                      juce::dontSendNotification);
                channelsCheckboxes[i]->onStateChange = [this, params, editor, i]() {
                    if (channelsCheckboxes[i]->getToggleState()) {
                        params->ghostNotesChannels.insert(i);
                    } else {
                        params->ghostNotesChannels.erase(i);
                    }
                    editor->updateGhostNotes();
                };
                addAndMakeVisible(channelsCheckboxes[i].get());
            }
        }

        const int totalHeight = titleHeight + (15 * rowHeight) + 20;
        setSize(width, totalHeight);
        titleLabel->setBounds(5, 10, width - 10, titleHeight);
        int ind = 0;
        for (int i = 0; i < 16; ++i) {
            if (i != chIndex) {
                int yPos = titleHeight + 10 + ind * rowHeight;
                channelsLabels[i]->setBounds(10, yPos, 160, rowHeight);
                channelsCheckboxes[i]->setBounds(160, yPos, 30, rowHeight);
                ind++;
            }
        }
    }

    void InstancesMenu::updateColors() {
        titleLabel->setColour(juce::Label::backgroundColourId, Theme::darker);
        for (int i = 0; i < 16; ++i) {
            if (i != chIndex) {
                channelsLabels[i]->setColour(juce::Label::backgroundColourId, Theme::darker);
            }
        }
    }
}