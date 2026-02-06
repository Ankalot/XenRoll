#include "XenRoll/EditRatiosMarksMenu.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
EditRatiosMarksMenu::EditRatiosMarksMenu(Parameters *params,
                                         AudioPluginAudioProcessorEditor *editor)
    : params(params), editor(editor) {
    setAlwaysOnTop(true);
    setWantsKeyboardFocus(false);
    setVisible(false);

    maxDenLabel = std::make_unique<juce::Label>();
    juce::Font currentFont = maxDenLabel->getFont();
    currentFont.setHeight(Theme::small_);
    maxDenLabel->setFont(currentFont);
    maxDenLabel->setText("Max denumerator", juce::dontSendNotification);
    maxDenLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(maxDenLabel.get());

    maxDenSlider = std::make_unique<juce::Slider>();
    maxDenSlider->setLookAndFeel(&editor->smallLF);
    maxDenSlider->setRange(params->min_maxDenRatiosMarks, params->max_maxDenRatiosMarks, 1);
    maxDenSlider->setValue(params->maxDenRatiosMarks);
    maxDenSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 30, rowHeight);
    maxDenSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    maxDenSlider->onValueChange = [this, params, editor]() {
        params->maxDenRatiosMarks = int(maxDenSlider->getValue());
        editor->editRatiosMarksMenuChanged();
    };
    addAndMakeVisible(maxDenSlider.get());

    goodEnoughErrorLabel = std::make_unique<juce::Label>();
    currentFont.setHeight(Theme::small_);
    goodEnoughErrorLabel->setFont(currentFont);
    goodEnoughErrorLabel->setText("Good enough error (cents)", juce::dontSendNotification);
    goodEnoughErrorLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(goodEnoughErrorLabel.get());

    goodEnoughErrorSlider = std::make_unique<juce::Slider>();
    goodEnoughErrorSlider->setLookAndFeel(&editor->smallLF);
    goodEnoughErrorSlider->setRange(params->min_goodEnoughErrorRatiosMarks,
                                    params->max_goodEnoughErrorRatiosMarks, 1);
    goodEnoughErrorSlider->setValue(params->goodEnoughErrorRatiosMarks);
    goodEnoughErrorSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 30, rowHeight);
    goodEnoughErrorSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    goodEnoughErrorSlider->onValueChange = [this, params, editor]() {
        params->goodEnoughErrorRatiosMarks = int(goodEnoughErrorSlider->getValue());
        editor->editRatiosMarksMenuChanged();
    };
    addAndMakeVisible(goodEnoughErrorSlider.get());

    autoCorrectLabel = std::make_unique<juce::Label>();
    currentFont.setHeight(Theme::small_);
    autoCorrectLabel->setFont(currentFont);
    autoCorrectLabel->setText("Auto-correct marks", juce::dontSendNotification);
    autoCorrectLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(autoCorrectLabel.get());

    autoCorrectCheckbox = std::make_unique<juce::ToggleButton>();
    autoCorrectCheckbox->setLookAndFeel(&editor->smallLF);
    autoCorrectCheckbox->setToggleState(params->autoCorrectRatiosMarks, juce::dontSendNotification);
    autoCorrectCheckbox->onStateChange = [this, params, editor]() {
        params->autoCorrectRatiosMarks = autoCorrectCheckbox->getToggleState();
        if (params->autoCorrectRatiosMarks) {
            editor->remakeKeys();
        }
    };
    addAndMakeVisible(autoCorrectCheckbox.get());

    int y = vertPadding;
    maxDenLabel->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;
    maxDenSlider->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    goodEnoughErrorLabel->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;
    goodEnoughErrorSlider->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    autoCorrectLabel->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    autoCorrectCheckbox->setBounds(horPadding, y, rowHeight, rowHeight);
    y += rowHeight + rowSkip;

    const int totalHeight = y + vertPadding;
    setSize(width, totalHeight);
}
} // namespace audio_plugin