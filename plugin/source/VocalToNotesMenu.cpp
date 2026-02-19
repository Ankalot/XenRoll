#include "XenRoll/VocalToNotesMenu.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
VocalToNotesMenu::VocalToNotesMenu(Parameters *params, AudioPluginAudioProcessorEditor *editor)
    : params(params), editor(editor) {
    setAlwaysOnTop(true);
    setWantsKeyboardFocus(false);
    setVisible(false);

    vocalToNotesDcentsLabel = std::make_unique<juce::Label>();
    juce::Font currentFont = vocalToNotesDcentsLabel->getFont();
    currentFont.setHeight(Theme::small_);
    vocalToNotesDcentsLabel->setFont(currentFont);
    vocalToNotesDcentsLabel->setText("Min pitch difference (cents)", juce::dontSendNotification);
    vocalToNotesDcentsLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(vocalToNotesDcentsLabel.get());

    vocalToNotesDcentsSlider = std::make_unique<juce::Slider>();
    vocalToNotesDcentsSlider->setLookAndFeel(&editor->smallLF);
    vocalToNotesDcentsSlider->setRange(params->min_vocalToNotesDCents,
                                       params->max_vocalToNotesDcents, 1);
    vocalToNotesDcentsSlider->setValue(params->vocalToNotesDCents);
    vocalToNotesDcentsSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 30, rowHeight);
    vocalToNotesDcentsSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    vocalToNotesDcentsSlider->onDragEnd = [this, params, editor]() {
        params->vocalToNotesDCents = static_cast<int>(vocalToNotesDcentsSlider->getValue());
    };
    addAndMakeVisible(vocalToNotesDcentsSlider.get());

    vocalToNotesKeySnapLabel = std::make_unique<juce::Label>();
    vocalToNotesKeySnapLabel->setText("Key snap", juce::dontSendNotification);
    vocalToNotesKeySnapLabel->setFont(currentFont);
    addAndMakeVisible(vocalToNotesKeySnapLabel.get());

    vocalToNotesKeySnapCheckbox = std::make_unique<juce::ToggleButton>();
    vocalToNotesKeySnapCheckbox->setToggleState(params->vocalToNotesKeySnap,
                                                juce::dontSendNotification);
    vocalToNotesKeySnapCheckbox->onStateChange = [this, params]() {
        params->vocalToNotesKeySnap = vocalToNotesKeySnapCheckbox->getToggleState();
    };
    addAndMakeVisible(vocalToNotesKeySnapCheckbox.get());

    vocalToNotesMakeBendsLabel = std::make_unique<juce::Label>();
    vocalToNotesMakeBendsLabel->setText("Make bended notes", juce::dontSendNotification);
    vocalToNotesMakeBendsLabel->setFont(currentFont);
    addAndMakeVisible(vocalToNotesMakeBendsLabel.get());

    vocalToNotesMakeBendsCheckbox = std::make_unique<juce::ToggleButton>();
    vocalToNotesMakeBendsCheckbox->setToggleState(params->vocalToNotesMakeBends,
                                                  juce::dontSendNotification);
    vocalToNotesMakeBendsCheckbox->onStateChange = [this, params]() {
        params->vocalToNotesMakeBends = vocalToNotesMakeBendsCheckbox->getToggleState();
    };
    addAndMakeVisible(vocalToNotesMakeBendsCheckbox.get());

    micGain_dBLabel = std::make_unique<juce::Label>();
    micGain_dBLabel->setFont(currentFont);
    micGain_dBLabel->setText("Microphone gain (dB)", juce::dontSendNotification);
    micGain_dBLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(micGain_dBLabel.get());

    micGain_dBSlider = std::make_unique<juce::Slider>();
    micGain_dBSlider->setLookAndFeel(&editor->smallLF);
    micGain_dBSlider->setRange(params->min_micGain_dB, params->max_micGain_dB, 0.1);
    micGain_dBSlider->setValue(params->micGain_dB);
    micGain_dBSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 45, rowHeight);
    micGain_dBSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    micGain_dBSlider->onDragEnd = [this, params, editor]() {
        params->micGain_dB = static_cast<float>(micGain_dBSlider->getValue());
    };
    addAndMakeVisible(micGain_dBSlider.get());

    int y = vertPadding;
    vocalToNotesDcentsLabel->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;
    vocalToNotesDcentsSlider->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    vocalToNotesKeySnapLabel->setBounds(horPadding, y, width - 2 * horPadding - rowHeight,
                                        rowHeight);
    vocalToNotesKeySnapCheckbox->setBounds(width - horPadding - rowHeight, y, rowHeight, rowHeight);
    y += rowHeight + rowSkip;

    vocalToNotesMakeBendsLabel->setBounds(horPadding, y, width - 2 * horPadding - rowHeight,
                                          rowHeight);
    vocalToNotesMakeBendsCheckbox->setBounds(width - horPadding - rowHeight, y, rowHeight,
                                             rowHeight);
    y += rowHeight + rowSkip;

    micGain_dBLabel->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;
    micGain_dBSlider->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;

    const int totalHeight = y + vertPadding;
    setSize(width, totalHeight);
}
} // namespace audio_plugin