#include "XenRoll/editor/menus/InstancesMenu.h"
#include "XenRoll/editor/PluginEditor.h"

namespace audio_plugin {

InstancesMenu::InstancesMenu(Parameters *params, AudioPluginAudioProcessorEditor *editor)
    : params(params), editor(editor) {
    setWantsKeyboardFocus(false);
    setVisible(false);

    if (params->getTuningType() == Parameters::TuningType::MPE) {
        myInd = params->instanceId;
    } else if (params->getTuningType() == Parameters::TuningType::MTS_ESP) {
        myInd = params->channelIndex;
    }

    // Setup title
    titleLabel = std::make_unique<juce::Label>();
    titleLabel->setText("View notes from instances", juce::dontSendNotification);
    font = titleLabel->getFont();
    font.setHeight(Theme::small_);
    titleLabel->setFont(font);
    titleLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel.get());

    // Setup viewport
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&contentComponent, false);

    setSize(width, height);

    // Setup content
    possibleInds = getPossibleInds();
    buildMenu();
}

void InstancesMenu::buildMenu() {
    instancesLabels.clear();
    instancesCheckboxes.clear();

    juce::String text;
    if (params->getTuningType() == Parameters::TuningType::MPE) {
        text = "Id ";
    } else if (params->getTuningType() == Parameters::TuningType::MTS_ESP) {
        text = "Ch ";
    }

    int num = 0;
    for (int ind : possibleInds) {
        auto instanceLabel = std::make_unique<juce::Label>();
        instanceLabel->setText(text + juce::String(ind + 1), juce::dontSendNotification);
        instanceLabel->setFont(font);
        contentComponent.addAndMakeVisible(instanceLabel.get());
        instancesLabels.push_back(std::move(instanceLabel));

        auto instanceCheckbox = std::make_unique<juce::ToggleButton>();
        if (params->getTuningType() == Parameters::TuningType::MPE) {
            instanceCheckbox->setToggleState(params->ghostNotesInstIds.contains(ind),
                                             juce::dontSendNotification);
            instanceCheckbox->onStateChange = [this, ind, num]() {
                if (this->instancesCheckboxes[num]->getToggleState()) {
                    params->ghostNotesInstIds.insert(ind);
                } else {
                    params->ghostNotesInstIds.erase(ind);
                }
                editor->updateGhostNotes();
            };
        } else if (params->getTuningType() == Parameters::TuningType::MTS_ESP) {
            instanceCheckbox->setToggleState(params->ghostNotesChannels.contains(ind),
                                             juce::dontSendNotification);
            instanceCheckbox->onStateChange = [this, ind, num]() {
                if (this->instancesCheckboxes[num]->getToggleState()) {
                    params->ghostNotesChannels.insert(ind);
                } else {
                    params->ghostNotesChannels.erase(ind);
                }
                editor->updateGhostNotes();
            };
        }
        contentComponent.addAndMakeVisible(instanceCheckbox.get());
        instancesCheckboxes.push_back(std::move(instanceCheckbox));
        num++;
    }

    const int contentWidth = viewport.getWidth();
    const int contentHeight = num * rowHeight;
    contentComponent.setSize(contentWidth, contentHeight);

    int yPos;
    for (int i = 0; i < num; ++i) {
        yPos = i * rowHeight;
        instancesLabels[i]->setBounds(viewportHorPadding, yPos,
                                      contentWidth - viewportHorPadding - 30, rowHeight);
        instancesCheckboxes[i]->setBounds(contentWidth - viewportHorPadding - 30, yPos, 22,
                                          rowHeight);
    }
}

std::set<int> InstancesMenu::getPossibleInds() {
    std::set<int> inds = editor->getAllInstancesIndexes();
    inds.erase(myInd);
    return inds;
}

} // namespace audio_plugin