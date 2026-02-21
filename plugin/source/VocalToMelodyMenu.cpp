#include "XenRoll/VocalToMelodyMenu.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
VocalToMelodyMenu::VocalToMelodyMenu(Parameters *params, AudioPluginAudioProcessorEditor *editor)
    : params(params), editor(editor) {
    setAlwaysOnTop(true);
    setWantsKeyboardFocus(false);
    setVisible(false);

    vocalToMelodyGenCurveLabel = std::make_unique<juce::Label>();
    juce::Font currentFont = vocalToMelodyGenCurveLabel->getFont();
    currentFont.setHeight(Theme::small_);
    vocalToMelodyGenCurveLabel->setText("GENERATE PITCH CURVE", juce::dontSendNotification);
    vocalToMelodyGenCurveLabel->setFont(currentFont);
    addAndMakeVisible(vocalToMelodyGenCurveLabel.get());

    vocalToMelodyGenCurveCheckbox = std::make_unique<juce::ToggleButton>();
    vocalToMelodyGenCurveCheckbox->setToggleState(params->vocalToMelodyGenCurve,
                                                  juce::dontSendNotification);
    vocalToMelodyGenCurveCheckbox->onStateChange = [this, params]() {
        bool newState = vocalToMelodyGenCurveCheckbox->getToggleState();
        params->vocalToMelodyGenCurve = newState;
        if (!newState && !params->vocalToMelodyGenNotes) {
            params->vocalToMelodyGenNotes = true;
            vocalToMelodyGenNotesCheckbox->setToggleState(true, juce::dontSendNotification);
        }
    };
    addAndMakeVisible(vocalToMelodyGenCurveCheckbox.get());

    vocalToMelodyDeleteCurveButton = std::make_unique<juce::TextButton>("Delete pitch curve");
    vocalToMelodyDeleteCurveButton->setLookAndFeel(&editor->smallLF);
    vocalToMelodyDeleteCurveButton->setClickingTogglesState(false);
    vocalToMelodyDeleteCurveButton->onClick = [this, editor]() { editor->clearPitchCurve(); };
    addAndMakeVisible(vocalToMelodyDeleteCurveButton.get());

    vocalToMelodyGenNotesLabel = std::make_unique<juce::Label>();
    vocalToMelodyGenNotesLabel->setText("GENERATE NOTES", juce::dontSendNotification);
    vocalToMelodyGenNotesLabel->setFont(currentFont);
    addAndMakeVisible(vocalToMelodyGenNotesLabel.get());

    vocalToMelodyGenNotesCheckbox = std::make_unique<juce::ToggleButton>();
    vocalToMelodyGenNotesCheckbox->setToggleState(params->vocalToMelodyGenNotes,
                                                  juce::dontSendNotification);
    vocalToMelodyGenNotesCheckbox->onStateChange = [this, params]() {
        bool newState = vocalToMelodyGenNotesCheckbox->getToggleState();
        params->vocalToMelodyGenNotes = newState;
        if (!newState && !params->vocalToMelodyGenCurve) {
            params->vocalToMelodyGenCurve = true;
            vocalToMelodyGenCurveCheckbox->setToggleState(true, juce::dontSendNotification);
        }
    };
    addAndMakeVisible(vocalToMelodyGenNotesCheckbox.get());

    vocalToMelodyKeySnapLabel = std::make_unique<juce::Label>();
    vocalToMelodyKeySnapLabel->setText("Key snap", juce::dontSendNotification);
    vocalToMelodyKeySnapLabel->setFont(currentFont);
    addAndMakeVisible(vocalToMelodyKeySnapLabel.get());

    vocalToMelodyKeySnapCheckbox = std::make_unique<juce::ToggleButton>();
    vocalToMelodyKeySnapCheckbox->setToggleState(params->vocalToMelodyKeySnap,
                                                 juce::dontSendNotification);
    vocalToMelodyKeySnapCheckbox->onStateChange = [this, params]() {
        params->vocalToMelodyKeySnap = vocalToMelodyKeySnapCheckbox->getToggleState();
    };
    addAndMakeVisible(vocalToMelodyKeySnapCheckbox.get());

    vocalToMelodyMinNoteDurationLabel = std::make_unique<juce::Label>();
    vocalToMelodyMinNoteDurationLabel->setFont(currentFont);
    vocalToMelodyMinNoteDurationLabel->setText("Min note duration (bars)",
                                               juce::dontSendNotification);
    vocalToMelodyMinNoteDurationLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(vocalToMelodyMinNoteDurationLabel.get());

    vocalToMelodyMinNoteDurationCombo = std::make_unique<juce::ComboBox>();
    vocalToMelodyMinNoteDurationCombo->addItem("1", 1);
    for (int i = 2; i <= 256; i *= 2) {
        vocalToMelodyMinNoteDurationCombo->addItem("1/" + juce::String(i), i);
    }
    vocalToMelodyMinNoteDurationCombo->setSelectedId(
        juce::roundToInt(1.0f / params->vocalToMelodyMinNoteDuration));
    vocalToMelodyMinNoteDurationCombo->setLookAndFeel(&editor->smallLF);
    vocalToMelodyMinNoteDurationCombo->onChange = [this, params, editor]() {
        params->vocalToMelodyMinNoteDuration =
            1.0f / vocalToMelodyMinNoteDurationCombo->getSelectedId();
    };
    addAndMakeVisible(vocalToMelodyMinNoteDurationCombo.get());

    vocalToMelodyDcentsLabel = std::make_unique<juce::Label>();
    vocalToMelodyDcentsLabel->setFont(currentFont);
    vocalToMelodyDcentsLabel->setText("Min pitch difference (cents)", juce::dontSendNotification);
    vocalToMelodyDcentsLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(vocalToMelodyDcentsLabel.get());

    vocalToMelodyDcentsSlider = std::make_unique<juce::Slider>();
    vocalToMelodyDcentsSlider->setLookAndFeel(&editor->smallLF);
    vocalToMelodyDcentsSlider->setRange(params->min_vocalToMelodyDCents,
                                        params->max_vocalToMelodyDcents, 1);
    vocalToMelodyDcentsSlider->setValue(params->vocalToMelodyDCents);
    vocalToMelodyDcentsSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 30, rowHeight);
    vocalToMelodyDcentsSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    vocalToMelodyDcentsSlider->onDragEnd = [this, params, editor]() {
        params->vocalToMelodyDCents = static_cast<int>(vocalToMelodyDcentsSlider->getValue());
    };
    addAndMakeVisible(vocalToMelodyDcentsSlider.get());

    vocalToMelodyKeySnapLabel = std::make_unique<juce::Label>();
    vocalToMelodyKeySnapLabel->setText("Key snap", juce::dontSendNotification);
    vocalToMelodyKeySnapLabel->setFont(currentFont);
    addAndMakeVisible(vocalToMelodyKeySnapLabel.get());

    vocalToMelodyKeySnapCheckbox = std::make_unique<juce::ToggleButton>();
    vocalToMelodyKeySnapCheckbox->setToggleState(params->vocalToMelodyKeySnap,
                                                 juce::dontSendNotification);
    vocalToMelodyKeySnapCheckbox->onStateChange = [this, params]() {
        params->vocalToMelodyKeySnap = vocalToMelodyKeySnapCheckbox->getToggleState();
    };
    addAndMakeVisible(vocalToMelodyKeySnapCheckbox.get());

    vocalToMelodyMakeBendsLabel = std::make_unique<juce::Label>();
    vocalToMelodyMakeBendsLabel->setText("Make bended notes", juce::dontSendNotification);
    vocalToMelodyMakeBendsLabel->setFont(currentFont);
    addAndMakeVisible(vocalToMelodyMakeBendsLabel.get());

    vocalToMelodyMakeBendsCheckbox = std::make_unique<juce::ToggleButton>();
    vocalToMelodyMakeBendsCheckbox->setToggleState(params->vocalToMelodyMakeBends,
                                                   juce::dontSendNotification);
    vocalToMelodyMakeBendsCheckbox->onStateChange = [this, params]() {
        params->vocalToMelodyMakeBends = vocalToMelodyMakeBendsCheckbox->getToggleState();
    };
    addAndMakeVisible(vocalToMelodyMakeBendsCheckbox.get());

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
    vocalToMelodyGenCurveLabel->setBounds(horPadding, y, width - 2 * horPadding - rowHeight,
                                          rowHeight);
    vocalToMelodyGenCurveCheckbox->setBounds(width - horPadding - rowHeight, y, rowHeight,
                                             rowHeight);
    y += rowHeight + rowSkip;

    vocalToMelodyDeleteCurveButton->setBounds(horPadding, y, width - 2 * horPadding, buttonHeight);
    y += buttonHeight + 2 * rowSkip;

    vocalToMelodyGenNotesLabel->setBounds(horPadding, y, width - 2 * horPadding - rowHeight,
                                          rowHeight);
    vocalToMelodyGenNotesCheckbox->setBounds(width - horPadding - rowHeight, y, rowHeight,
                                             rowHeight);
    y += rowHeight + rowSkip;

    vocalToMelodyMinNoteDurationLabel->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;
    vocalToMelodyMinNoteDurationCombo->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    vocalToMelodyDcentsLabel->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;
    vocalToMelodyDcentsSlider->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight + rowSkip;

    vocalToMelodyKeySnapLabel->setBounds(horPadding, y, width - 2 * horPadding - rowHeight,
                                         rowHeight);
    vocalToMelodyKeySnapCheckbox->setBounds(width - horPadding - rowHeight, y, rowHeight,
                                            rowHeight);
    y += rowHeight + rowSkip;

    vocalToMelodyMakeBendsLabel->setBounds(horPadding, y, width - 2 * horPadding - rowHeight,
                                           rowHeight);
    vocalToMelodyMakeBendsCheckbox->setBounds(width - horPadding - rowHeight, y, rowHeight,
                                              rowHeight);
    y += rowHeight + 2 * rowSkip;

    micGain_dBLabel->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;
    micGain_dBSlider->setBounds(horPadding, y, width - 2 * horPadding, rowHeight);
    y += rowHeight;

    const int totalHeight = y + vertPadding;
    setSize(width, totalHeight);
}
} // namespace audio_plugin