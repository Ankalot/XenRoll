#include "XenRoll/GenNewKeysMenu.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
GenNewKeysMenu::GenNewKeysMenu(Parameters *params, AudioPluginAudioProcessorEditor *editor)
    : params(params), editor(editor) {
    setAlwaysOnTop(true);
    setWantsKeyboardFocus(false);
    setVisible(false);

    numNewKeysLabel = std::make_unique<juce::Label>();
    juce::Font currentFont = numNewKeysLabel->getFont();
    currentFont.setHeight(Theme::small_);
    numNewKeysLabel->setFont(currentFont);
    numNewKeysLabel->setText("Num of new keys", juce::dontSendNotification);
    numNewKeysLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(numNewKeysLabel.get());

    numNewKeysSlider = std::make_unique<juce::Slider>();
    numNewKeysSlider->setLookAndFeel(&editor->smallLF);
    numNewKeysSlider->setRange(params->min_num_new_notes, params->max_num_new_notes, 1);
    numNewKeysSlider->setValue(params->numNewGenKeys);
    numNewKeysSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 30, rowHeight);
    numNewKeysSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    numNewKeysSlider->onDragEnd = [this, params, editor]() {
        params->numNewGenKeys = int(numNewKeysSlider->getValue());
        if (params->generateNewKeys) {
            editor->remakeKeys();
        }
    };
    addAndMakeVisible(numNewKeysSlider.get());

    genNewKeysTacticsLabel = std::make_unique<juce::Label>();
    genNewKeysTacticsLabel->setFont(currentFont);
    genNewKeysTacticsLabel->setText("Tactics", juce::dontSendNotification);
    genNewKeysTacticsLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(genNewKeysTacticsLabel.get());

    genNewKeysTacticsCombo = std::make_unique<juce::ComboBox>();
    genNewKeysTacticsCombo->setLookAndFeel(&editor->smallLF);
    genNewKeysTacticsCombo->addItemList(params->getGenNewKeysTacticsNames(), 1);
    genNewKeysTacticsCombo->setSelectedId(static_cast<int>(params->genNewKeysTactics));
    genNewKeysTacticsCombo->onChange = [this, params, editor]() {
        params->genNewKeysTactics =
            static_cast<Parameters::GenNewKeysTactics>(genNewKeysTacticsCombo->getSelectedId());
        if (params->generateNewKeys) {
            editor->remakeKeys();
        }
    };
    addAndMakeVisible(genNewKeysTacticsCombo.get());

    minDistExistNewKeysLabel = std::make_unique<juce::Label>();
    minDistExistNewKeysLabel->setFont(currentFont);
    minDistExistNewKeysLabel->setText("Min dist from existing to new keys",
                                      juce::dontSendNotification);
    minDistExistNewKeysLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(minDistExistNewKeysLabel.get());

    minDistExistNewKeysSlider = std::make_unique<juce::Slider>();
    minDistExistNewKeysSlider->setLookAndFeel(&editor->smallLF);
    minDistExistNewKeysSlider->setRange(params->min_minDistExistNewKeys,
                                        params->max_minDistExistNewKeys, 1);
    minDistExistNewKeysSlider->setValue(params->minDistExistNewKeys);
    minDistExistNewKeysSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, rowHeight);
    minDistExistNewKeysSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    minDistExistNewKeysSlider->onDragEnd = [this, params, editor]() {
        params->minDistExistNewKeys = int(minDistExistNewKeysSlider->getValue());
        if (params->generateNewKeys) {
            editor->remakeKeys();
        }
    };
    addAndMakeVisible(minDistExistNewKeysSlider.get());

    minDistBetweenNewKeysLabel = std::make_unique<juce::Label>();
    minDistBetweenNewKeysLabel->setFont(currentFont);
    minDistBetweenNewKeysLabel->setText("Min dist between new keys", juce::dontSendNotification);
    minDistBetweenNewKeysLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(minDistBetweenNewKeysLabel.get());

    minDistBetweenNewKeysSlider = std::make_unique<juce::Slider>();
    minDistBetweenNewKeysSlider->setLookAndFeel(&editor->smallLF);
    minDistBetweenNewKeysSlider->setRange(params->min_minDistBetweenNewKeys,
                                          params->max_minDistBetweenNewKeys, 1);
    minDistBetweenNewKeysSlider->setValue(params->minDistBetweenNewKeys);
    minDistBetweenNewKeysSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, rowHeight);
    minDistBetweenNewKeysSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    minDistBetweenNewKeysSlider->onDragEnd = [this, params, editor]() {
        params->minDistBetweenNewKeys = int(minDistBetweenNewKeysSlider->getValue());
        if (params->generateNewKeys) {
            editor->remakeKeys();
        }
    };
    addAndMakeVisible(minDistBetweenNewKeysSlider.get());

    int y = vertPadding;
    numNewKeysLabel->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;
    numNewKeysSlider->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    genNewKeysTacticsLabel->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;
    genNewKeysTacticsCombo->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    minDistExistNewKeysLabel->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;
    minDistExistNewKeysSlider->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    minDistBetweenNewKeysLabel->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;
    minDistBetweenNewKeysSlider->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;

    const int totalHeight = y + vertPadding;
    setSize(width, totalHeight);
}
} // namespace audio_plugin