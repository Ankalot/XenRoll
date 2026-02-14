#include "XenRoll/DissonancePanel.h"

namespace audio_plugin {
DissonancePanel::DissonancePanel(Parameters *params,
                                 std::shared_ptr<DissonanceMeter> dissonanceMeter)
    : params(params), dissonanceMeter(dissonanceMeter) {
    setVisible(false);
    setAlwaysOnTop(true);

    // Other
    partialsFileChooser = std::make_unique<juce::FileChooser>(
        "Import/export partials", juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.txt");

    // Create plots
    partialsPlot = std::make_unique<PartialsPlot>(params, params->plotPartialsInterp);
    addAndMakeVisible(partialsPlot.get());

    dissonancePlot = std::make_unique<DissonancePlot>(params, dissonanceMeter);
    addAndMakeVisible(dissonancePlot.get());

    // Create components that control partials plot
    plotPartialsInterpButton = std::make_unique<SVGButton>(
        BinaryData::Transpose_svg, BinaryData::Transpose_svgSize, true, params->plotPartialsInterp,
        std::string("Plot partials for all pitches. If some pitch has no partials calculated, ") +
            "it will use the transposed partials of the closest recorded pitch");
    addAndMakeVisible(plotPartialsInterpButton.get());
    plotPartialsInterpButton->onClick = [this, params](const juce::MouseEvent &me) {
        params->plotPartialsInterp = !params->plotPartialsInterp;
        partialsPlot->setInterpMode(params->plotPartialsInterp);
        return true;
    };

    plotPartialsOctaveInput = std::make_unique<IntegerInput>(params->plotPartialsTotalCents / 1200,
                                                             0, params->num_octaves);
    plotPartialsOctaveInput->onValueChanged = [this, params](int newValue) {
        if (!ignoreUpdatePartials)
            updatePartialsPlotTotalCents(newValue * 1200 + plotPartialsCentsInput->getValue(), 1);
    };
    addAndMakeVisible(plotPartialsOctaveInput.get());

    plotPartialsOctaveLabel = std::make_unique<juce::Label>();
    plotPartialsOctaveLabel->setText("octave", juce::dontSendNotification);
    juce::Font currentFont = plotPartialsOctaveLabel->getFont();
    currentFont.setHeight(Theme::big);
    plotPartialsOctaveLabel->setFont(currentFont);
    addAndMakeVisible(plotPartialsOctaveLabel.get());

    plotPartialsCentsInput =
        std::make_unique<IntegerInput>(params->plotPartialsTotalCents % 1200, 0, 1199);
    plotPartialsCentsInput->onValueChanged = [this, params](int newValue) {
        if (!ignoreUpdatePartials)
            updatePartialsPlotTotalCents(plotPartialsOctaveInput->getValue() * 1200 + newValue, 2);
    };
    addAndMakeVisible(plotPartialsCentsInput.get());

    plotPartialsCentsLabel = std::make_unique<juce::Label>();
    plotPartialsCentsLabel->setText("cents", juce::dontSendNotification);
    plotPartialsCentsLabel->setFont(currentFont);
    addAndMakeVisible(plotPartialsCentsLabel.get());

    plotPartialsTotalCentsSlider = std::make_unique<juce::Slider>();
    plotPartialsTotalCentsSlider->setRange(params->min_plotPartialsTotalCents,
                                           params->max_plotPartialsTotalCents, 1.0);
    plotPartialsTotalCentsSlider->setValue(params->plotPartialsTotalCents);
    plotPartialsTotalCentsSlider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    plotPartialsTotalCentsSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    plotPartialsTotalCentsSlider->onValueChange = [this, params]() {
        if (!ignoreUpdatePartials)
            updatePartialsPlotTotalCents(int(plotPartialsTotalCentsSlider->getValue()), 3);
    };
    addAndMakeVisible(plotPartialsTotalCentsSlider.get());

    removePartialsButton = std::make_unique<SVGButton>(
        BinaryData::Broom_svg, BinaryData::Broom_svgSize, false, false,
        std::string("Remove partials of tone with specified octave and cents"));
    addAndMakeVisible(removePartialsButton.get());
    removePartialsButton->onClick = [this, params](const juce::MouseEvent &me) {
        bool removed = params->remove_partials(params->plotPartialsTotalCents);
        if (removed) {
            partialsVec partials = {};
            int partialsTotalCents = 5700;
            if (!params->get_tonesPartials().empty()) {
                partials = params->get_tonesPartials().begin()->second;
                partialsTotalCents = params->get_tonesPartials().begin()->first;
            }
            this->dissonanceMeter->setPartials(partials, partialsTotalCents);
            partialsPlot->updateTonesPartials();
            dissonancePlot->updateDissonanceCurve();
        }
        return false;
    };

    trashButton =
        std::make_unique<SVGButton>(BinaryData::Trash_svg, BinaryData::Trash_svgSize, false, false,
                                    std::string("Delete partials from all tones"));
    addAndMakeVisible(trashButton.get());
    trashButton->onClick = [this, params](const juce::MouseEvent &me) {
        juce::NativeMessageBox::showOkCancelBox(
            juce::AlertWindow::WarningIcon, "Delete All Partials?",
            "This will permanently remove all partials data from every tone.", this,
            juce::ModalCallbackFunction::create([this, params](int result) {
                if (result) { // OK clicked
                    params->set_tonesPartials({});
                    this->dissonanceMeter->setPartials({}, 5700);
                    partialsPlot->updateTonesPartials();
                    dissonancePlot->updateDissonanceCurve();
                }
            }));
        return false;
    };

    importPartialsButton = std::make_unique<SVGButton>(
        BinaryData::Import_partials_svg, BinaryData::Import_partials_svgSize, false, false,
        std::string("Import .txt file with partials data.\n"
                    "Example: {{5700, {{440.0, 1.0}}}} (This example contains one tone with pitch "
                    "5700 in totalCents, this tone has single partial with 440.0 Hz frequency "
                    "and 1.0 amplitude (linear)\n"
                    "totalCents = octave*1200 + cents; 4oct 900¢ = A4 note.\n"));
    addAndMakeVisible(importPartialsButton.get());
    importPartialsButton->onClick = [this, params](const juce::MouseEvent &me) {
        partialsFileChooser.get()->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser &fc) {
                juce::File txtFile = fc.getResult();
                if (!txtFile.existsAsFile())
                    return; // User cancelled

                juce::String fileContent = txtFile.loadFileAsString();

                std::map<int, std::vector<std::pair<float, float>>> parsedPartials;

                // Remove whitespace and normalize
                juce::String normalized =
                    fileContent.replace("\n", "").replace(" ", "").replace("\t", "");

                // Validate format
                if (!normalized.startsWithChar('{') || !normalized.endsWithChar('}')) {
                    return;
                }

                // Extract inner content
                juce::String innerContent = normalized.substring(1, normalized.length() - 1);

                // Parse tone entries
                int pos = 0;
                while (pos < innerContent.length()) {
                    // Find next tone entry
                    int start = innerContent.indexOf(pos, "{");
                    if (start == -1)
                        break;

                    int end = innerContent.indexOf(start + 1, "}}");
                    if (end == -1)
                        break;

                    juce::String entry = innerContent.substring(start, end + 2);
                    pos = end + 2;

                    // Parse totalCents
                    int centsEnd = entry.indexOf(1, ",");
                    if (centsEnd == -1)
                        continue;

                    int totalCents = entry.substring(1, centsEnd).getIntValue();

                    // Parse partials
                    int partialsStart = entry.indexOf(centsEnd, "{");
                    if (partialsStart == -1)
                        continue;

                    juce::String partialsStr =
                        entry.substring(partialsStart + 1, entry.length() - 1);

                    std::vector<std::pair<float, float>> partials;
                    int partialPos = 0;

                    while (partialPos < partialsStr.length()) {
                        int pStart = partialsStr.indexOf(partialPos, "{");
                        if (pStart == -1)
                            break;

                        int pEnd = partialsStr.indexOf(pStart + 1, "}");
                        if (pEnd == -1)
                            break;

                        juce::String partial = partialsStr.substring(pStart + 1, pEnd);
                        int commaPos = partial.indexOf(0, ",");

                        if (commaPos != -1) {
                            partials.emplace_back(partial.substring(0, commaPos).getFloatValue(),
                                                  partial.substring(commaPos + 1).getFloatValue());
                        }

                        partialPos = pEnd + 1;
                    }

                    if (!partials.empty()) {
                        parsedPartials[totalCents] = std::move(partials);
                    }
                }

                this->params->set_tonesPartials(parsedPartials);
                partialsVec partials = {};
                int partialsTotalCents = 5700;
                if (!parsedPartials.empty()) {
                    partials = parsedPartials.begin()->second;
                    partialsTotalCents = parsedPartials.begin()->first;
                }
                this->dissonanceMeter->setPartials(partials, partialsTotalCents);
                this->partialsPlot->updateTonesPartials();
                this->dissonancePlot->updateDissonanceCurve();
            });
        return false;
    };

    exportPartialsButton = std::make_unique<SVGButton>(
        BinaryData::Export_partials_svg, BinaryData::Export_partials_svgSize, false, false,
        std::string("Export partials to a .txt file"));
    addAndMakeVisible(exportPartialsButton.get());
    params->get_tonesPartials();
    exportPartialsButton->onClick = [this, params](const juce::MouseEvent &me) {
        partialsFileChooser.get()->launchAsync(
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [allPartials = params->get_tonesPartials()](const juce::FileChooser &fc) {
                juce::File txtFile = fc.getResult();
                txtFile.deleteFile();

                juce::FileOutputStream outputStream(txtFile);
                if (!outputStream.openedOk()) {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon, "Error",
                        "Failed to create file: " + txtFile.getFullPathName(), "OK");
                    return;
                }

                outputStream.writeText("{\n", false, false, nullptr);
                int numTones = allPartials.size();
                int toneInd = 0;
                for (const auto &[totalCents, partials] : allPartials) {
                    outputStream.writeText("    {" + juce::String(totalCents) + ", {\n", false,
                                           false, nullptr);

                    int numPartials = partials.size();
                    int partialInd = 0;
                    for (const auto &[freq, amp] : partials) {
                        outputStream.writeText("        {" + juce::String(freq, 3) + ", " +
                                                   juce::String(amp, 6) + "}",
                                               false, false, nullptr);

                        if (partialInd + 1 != numPartials)
                            outputStream.writeText(",", false, false, nullptr);
                        outputStream.writeText("\n", false, false, nullptr);
                        partialInd++;
                    }

                    outputStream.writeText("    }}", false, false, nullptr);

                    if (toneInd + 1 != numTones)
                        outputStream.writeText(",", false, false, nullptr);
                    outputStream.writeText("\n", false, false, nullptr);
                    toneInd++;
                }
                outputStream.writeText("}\n", false, false, nullptr);

                outputStream.flush();
            });
        return false;
    };

    // Create components that control dissonance plot
    plotDissonanceOctaveInput = std::make_unique<IntegerInput>(
        params->plotDissonanceTotalCents / 1200, 0, params->num_octaves - 1);
    plotDissonanceOctaveInput->onValueChanged = [this, params](int newValue) {
        if (!ignoreUpdateDissonance)
            updateDissonancePlotTotalCents(newValue * 1200 + plotDissonanceCentsInput->getValue(),
                                           1);
    };
    addAndMakeVisible(plotDissonanceOctaveInput.get());

    plotDissonanceOctaveLabel = std::make_unique<juce::Label>();
    plotDissonanceOctaveLabel->setText("octave", juce::dontSendNotification);
    plotDissonanceOctaveLabel->setFont(currentFont);
    addAndMakeVisible(plotDissonanceOctaveLabel.get());

    plotDissonanceCentsInput =
        std::make_unique<IntegerInput>(params->plotDissonanceTotalCents % 1200, 0, 1199);
    plotDissonanceCentsInput->onValueChanged = [this, params](int newValue) {
        if (!ignoreUpdateDissonance)
            updateDissonancePlotTotalCents(plotDissonanceOctaveInput->getValue() * 1200 + newValue,
                                           2);
    };
    addAndMakeVisible(plotDissonanceCentsInput.get());

    plotDissonanceCentsLabel = std::make_unique<juce::Label>();
    plotDissonanceCentsLabel->setText("cents", juce::dontSendNotification);
    plotDissonanceCentsLabel->setFont(currentFont);
    addAndMakeVisible(plotDissonanceCentsLabel.get());

    plotDissonanceTotalCentsSlider = std::make_unique<juce::Slider>();
    plotDissonanceTotalCentsSlider->setRange(params->min_plotDissonanceTotalCents,
                                             params->max_plotDissonanceTotalCents, 1.0);
    plotDissonanceTotalCentsSlider->setValue(params->plotDissonanceTotalCents);
    plotDissonanceTotalCentsSlider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    plotDissonanceTotalCentsSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    plotDissonanceTotalCentsSlider->onValueChange = [this, params]() {
        if (!ignoreUpdateDissonance)
            updateDissonancePlotTotalCents(int(plotDissonanceTotalCentsSlider->getValue()), 3);
    };
    addAndMakeVisible(plotDissonanceTotalCentsSlider.get());

    compactnessLabel = std::make_unique<juce::Label>();
    compactnessLabel->setText("Compactness", juce::dontSendNotification);
    compactnessLabel->setFont(currentFont);
    addAndMakeVisible(compactnessLabel.get());

    roughCompactFracSlider = std::make_unique<juce::Slider>();
    roughCompactFracSlider->setRange(params->min_roughCompactFrac, params->max_roughCompactFrac,
                                     0.01);
    roughCompactFracSlider->setValue(params->roughCompactFrac);
    roughCompactFracSlider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    roughCompactFracSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    roughCompactFracSlider->onValueChange = [this, params]() {
        params->roughCompactFrac = roughCompactFracSlider->getValue();
        this->dissonanceMeter->setAlpha(params->roughCompactFrac);
        dissonancePlot->updateDissonanceCurve();
    };
    addAndMakeVisible(roughCompactFracSlider.get());

    roughnessLabel = std::make_unique<juce::Label>();
    roughnessLabel->setText("Roughness", juce::dontSendNotification);
    roughnessLabel->setFont(currentFont);
    addAndMakeVisible(roughnessLabel.get());

    dissonancePowLabel = std::make_unique<juce::Label>();
    dissonancePowLabel->setText("Pow:", juce::dontSendNotification);
    dissonancePowLabel->setFont(currentFont);
    addAndMakeVisible(dissonancePowLabel.get());

    dissonancePowSlider = std::make_unique<juce::Slider>();
    dissonancePowSlider->setRange(params->min_dissonancePow, params->max_dissonancePow, 0.1);
    dissonancePowSlider->setValue(params->dissonancePow);
    dissonancePowSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, sliderTextBoxWidth,
                                         lowComponentHeight);
    dissonancePowSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    dissonancePowSlider->onValueChange = [this, params]() {
        params->dissonancePow = dissonancePowSlider->getValue();
        this->dissonanceMeter->setBeta(params->dissonancePow);
        dissonancePlot->updateDissonanceCurve();
    };
    addAndMakeVisible(dissonancePowSlider.get());

    // Create components that control finding partials
    refreshButton =
        std::make_unique<SVGButton>(BinaryData::Refresh_svg, BinaryData::Refresh_svgSize, false,
                                    false, std::string("Refresh plots"));
    addAndMakeVisible(refreshButton.get());
    refreshButton->onClick = [this, params](const juce::MouseEvent &me) {
        partialsVec partials = {};
        int partialsTotalCents = 5700;
        if (!params->get_tonesPartials().empty()) {
            partials = params->get_tonesPartials().begin()->second;
            partialsTotalCents = params->get_tonesPartials().begin()->first;
        }
        this->dissonanceMeter->setPartials(partials, partialsTotalCents);
        partialsPlot->updateTonesPartials();
        dissonancePlot->updateDissonanceCurve();
        return false;
    };

    switchFindPartialsModeButton = std::make_unique<SVGButton>(
        BinaryData::Turn_on_svg, BinaryData::Turn_on_svgSize, true, params->findPartialsMode.load(),
        std::string(
            "1. Place this plugin at the end of the fx chain. ENABLE MIDI PASSING THROUGH IN EACH "
            "PLUGIN IN FX CHAIN OR ROUTE MIDI INPUT FROM TRACK TO THIS PLUGIN IN DAW.\n") +
            "2. Turn on this button.\n" +
            "3. Now this plugin works as partials finder. The usual 12EDO tuning system is" +
            " set (lowest midi key = C0) on midi channel that is used by this instance. " +
            "Play ONE note at a time (using DAW's piano roll for example), hold it down until" +
            " decay starts or the sound stops changing.\n" +
            "4. If you don't like the result, tweak settings. " +
            "For example, if there are no partials or very few of them, then start by reducing dB "
            "threadshold.\n");
    addAndMakeVisible(switchFindPartialsModeButton.get());
    switchFindPartialsModeButton->onClick = [this, params](const juce::MouseEvent &me) {
        params->findPartialsMode.store(!params->findPartialsMode.load());
        return true;
    };

    dBThresholdLabel = std::make_unique<juce::Label>();
    dBThresholdLabel->setText("dB Threshold:", juce::dontSendNotification);
    dBThresholdLabel->setFont(currentFont);
    addAndMakeVisible(dBThresholdLabel.get());

    dBThresholdSlider = std::make_unique<juce::Slider>();
    dBThresholdSlider->setRange(params->min_findPartialsdBThreshold,
                                params->max_findPartialsdBThreshold, 1.0);
    dBThresholdSlider->setValue(params->findPartialsdBThreshold.load());
    dBThresholdSlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, sliderTextBoxWidth,
                                       lowComponentHeight);
    dBThresholdSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    dBThresholdSlider->onValueChange = [this, params]() {
        params->findPartialsdBThreshold.store(dBThresholdSlider->getValue());
    };
    addAndMakeVisible(dBThresholdSlider.get());

    strategyLabel = std::make_unique<juce::Label>();
    strategyLabel->setText("Strategy for window pos:", juce::dontSendNotification);
    strategyLabel->setFont(currentFont);
    addAndMakeVisible(strategyLabel.get());

    strategyComboBox = std::make_unique<juce::ComboBox>();
    strategyComboBox->addItem("Max spectral flatness (all tones)",
                              static_cast<int>(PartialsFinder::PosFindStrat::maxSpectralFlatness));
    strategyComboBox->addItem("Median RMS (sustained tones)",
                              static_cast<int>(PartialsFinder::PosFindStrat::medianRMS));
    strategyComboBox->addItem("Midrange RMS (percussive tones)",
                              static_cast<int>(PartialsFinder::PosFindStrat::midrangeRMS));
    strategyComboBox->addItem("Min RMS fluctuation (sustained tones)",
                              static_cast<int>(PartialsFinder::PosFindStrat::minRMSfluct));
    strategyComboBox->addItem("Peak sample (percussive tones)",
                              static_cast<int>(PartialsFinder::PosFindStrat::peakSample));
    strategyComboBox->setSelectedId(static_cast<int>(params->findPartialsStrat.load()));
    strategyComboBox->onChange = [this, params]() {
        params->findPartialsStrat.store(
            static_cast<PartialsFinder::PosFindStrat>(strategyComboBox->getSelectedId()));
    };
    addAndMakeVisible(strategyComboBox.get());

    fftSizeLabel = std::make_unique<juce::Label>();
    fftSizeLabel->setText("FFT size:", juce::dontSendNotification);
    fftSizeLabel->setFont(currentFont);
    addAndMakeVisible(fftSizeLabel.get());

    fftSizeComboBox = std::make_unique<juce::ComboBox>();
    fftSizeComboBox->addItem("4096", 4096);
    fftSizeComboBox->addItem("8192", 8192);
    fftSizeComboBox->addItem("16384", 16384);
    fftSizeComboBox->setSelectedId(static_cast<int>(params->findPartialsFFTSize.load()));
    fftSizeComboBox->onChange = [this, params]() {
        params->findPartialsFFTSize.store(fftSizeComboBox->getSelectedId());
    };
    addAndMakeVisible(fftSizeComboBox.get());
}

DissonancePanel::~DissonancePanel() = default;

void DissonancePanel::updateDissonancePlotTotalCents(int newTotalCents, int ind) {
    ignoreUpdateDissonance = true;
    if ((ind == 1) || (ind == 2)) {
        plotDissonanceTotalCentsSlider->setValue(double(newTotalCents));
    } else if (ind == 3) {
        plotDissonanceOctaveInput->setValue(newTotalCents / 1200);
        plotDissonanceCentsInput->setValue(newTotalCents % 1200);
    }
    params->plotDissonanceTotalCents = newTotalCents;
    ignoreUpdateDissonance = false;

    dissonancePlot->updateTotalCents();
}

void DissonancePanel::updatePartialsPlotTotalCents(int newTotalCents, int ind) {
    ignoreUpdatePartials = true;
    if ((ind == 1) || (ind == 2)) {
        plotPartialsTotalCentsSlider->setValue(double(newTotalCents));
    } else if (ind == 3) {
        plotPartialsOctaveInput->setValue(newTotalCents / 1200);
        plotPartialsCentsInput->setValue(newTotalCents % 1200);
    }
    params->plotPartialsTotalCents = newTotalCents;
    ignoreUpdatePartials = false;

    partialsPlot->updateTotalCents();
}

float getTextWidth(const juce::String &text, const juce::Font &font) {
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, text, 0, 0);
    return glyphs.getBoundingBox(0, -1, true).getWidth();
}

void DissonancePanel::resized() {
    int width = getWidth();
    int height = getHeight();

    // Top left
    auto topLeftBounds = getLocalBounds()
                             .removeFromTop(height - 2 * padding - lowComponentHeight)
                             .removeFromLeft(width / 2)
                             .reduced(padding);

    partialsPlot->setBounds(topLeftBounds.removeFromTop(topLeftBounds.getHeight() - 2 * padding -
                                                        2 * lowComponentHeight));
    topLeftBounds.removeFromTop(padding);

    auto topLeftFirstRow = topLeftBounds.removeFromTop(lowComponentHeight);
    topLeftBounds.removeFromTop(padding);

    plotPartialsInterpButton->setBounds(topLeftFirstRow.removeFromLeft(lowComponentHeight));
    topLeftFirstRow.removeFromLeft(padding);

    plotPartialsOctaveLabel->setBounds(topLeftFirstRow.removeFromLeft(
        int(getTextWidth(plotPartialsOctaveLabel->getText(), plotPartialsOctaveLabel->getFont()))));
    plotPartialsOctaveInput->setBounds(topLeftFirstRow.removeFromLeft(octaveInputWidth));
    topLeftFirstRow.removeFromLeft(padding);

    plotPartialsCentsLabel->setBounds(topLeftFirstRow.removeFromLeft(
        int(getTextWidth(plotPartialsCentsLabel->getText(), plotPartialsCentsLabel->getFont()))));
    plotPartialsCentsInput->setBounds(topLeftFirstRow.removeFromLeft(centsInputWidth));
    topLeftFirstRow.removeFromLeft(padding);

    plotPartialsTotalCentsSlider->setBounds(topLeftFirstRow);

    auto topLeftSecondRow = topLeftBounds;

    removePartialsButton->setBounds(topLeftSecondRow.removeFromLeft(lowComponentHeight));
    topLeftSecondRow.removeFromLeft(padding);

    trashButton->setBounds(topLeftSecondRow.removeFromLeft(lowComponentHeight));
    topLeftSecondRow.removeFromLeft(padding);

    importPartialsButton->setBounds(topLeftSecondRow.removeFromLeft(lowComponentHeight));
    topLeftSecondRow.removeFromLeft(padding);

    exportPartialsButton->setBounds(topLeftSecondRow.removeFromLeft(lowComponentHeight));
    topLeftSecondRow.removeFromLeft(padding);

    // Top right
    auto topRightBounds = getLocalBounds()
                              .removeFromTop(height - 2 * padding - lowComponentHeight)
                              .removeFromRight(width / 2)
                              .reduced(padding);

    dissonancePlot->setBounds(topRightBounds.removeFromTop(topRightBounds.getHeight() -
                                                           2 * padding - 2 * lowComponentHeight));
    topRightBounds.removeFromTop(padding);

    auto topRightFirstRow = topRightBounds.removeFromTop(lowComponentHeight);
    topRightBounds.removeFromTop(padding);

    plotDissonanceOctaveLabel->setBounds(topRightFirstRow.removeFromLeft(int(
        getTextWidth(plotDissonanceOctaveLabel->getText(), plotDissonanceOctaveLabel->getFont()))));
    plotDissonanceOctaveInput->setBounds(topRightFirstRow.removeFromLeft(octaveInputWidth));
    topRightFirstRow.removeFromLeft(padding);

    plotDissonanceCentsLabel->setBounds(topRightFirstRow.removeFromLeft(int(
        getTextWidth(plotDissonanceCentsLabel->getText(), plotDissonanceCentsLabel->getFont()))));
    plotDissonanceCentsInput->setBounds(topRightFirstRow.removeFromLeft(centsInputWidth));
    topRightFirstRow.removeFromLeft(padding);

    plotDissonanceTotalCentsSlider->setBounds(topRightFirstRow);

    compactnessLabel->setBounds(topRightBounds.removeFromLeft(
        int(getTextWidth(compactnessLabel->getText(), compactnessLabel->getFont()))));
    roughCompactFracSlider->setBounds(topRightBounds.removeFromLeft(roughCompactFracSliderWidth));
    roughnessLabel->setBounds(topRightBounds.removeFromLeft(
        int(getTextWidth(roughnessLabel->getText(), roughnessLabel->getFont()))));
    topRightFirstRow.removeFromLeft(padding);

    dissonancePowLabel->setBounds(topRightBounds.removeFromLeft(
        int(getTextWidth(dissonancePowLabel->getText(), dissonancePowLabel->getFont()))));
    dissonancePowSlider->setBounds(topRightBounds);

    // Bottom
    auto bottomBounds =
        getLocalBounds().removeFromBottom(2 * padding + lowComponentHeight).reduced(padding);

    refreshButton->setBounds(bottomBounds.removeFromLeft(lowComponentHeight));
    bottomBounds.removeFromLeft(padding);

    switchFindPartialsModeButton->setBounds(bottomBounds.removeFromLeft(lowComponentHeight));
    bottomBounds.removeFromLeft(padding);

    strategyLabel->setBounds(bottomBounds.removeFromLeft(
        int(getTextWidth(strategyLabel->getText(), strategyLabel->getFont()))));
    strategyComboBox->setBounds(bottomBounds.removeFromLeft(strategyComboBoxWidth));
    bottomBounds.removeFromLeft(padding);

    fftSizeLabel->setBounds(bottomBounds.removeFromLeft(
        int(getTextWidth(fftSizeLabel->getText(), fftSizeLabel->getFont()))));
    fftSizeComboBox->setBounds(bottomBounds.removeFromLeft(fftSizeComboBoxWidth));
    bottomBounds.removeFromLeft(padding);

    dBThresholdLabel->setBounds(bottomBounds.removeFromLeft(
        int(getTextWidth(dBThresholdLabel->getText(), dBThresholdLabel->getFont()))));
    dBThresholdSlider->setBounds(bottomBounds);
}

void DissonancePanel::paint(juce::Graphics &g) {
    g.fillAll(Theme::darker);

    int width = getWidth();
    int height = getHeight();
    float lineThickness = Theme::wider;

    g.setColour(Theme::darkest);
    g.drawLine(width / 2.0f, float(padding), width / 2.0f,
               height - 2 * padding - lowComponentHeight, lineThickness);
    g.drawLine(float(padding), height - lineThickness / 2, float(width - padding),
               height - lineThickness / 2, lineThickness);
    g.drawLine(float(padding), height - 2 * padding - lowComponentHeight, float(width - padding),
               height - 2 * padding - lowComponentHeight, lineThickness);
}
} // namespace audio_plugin