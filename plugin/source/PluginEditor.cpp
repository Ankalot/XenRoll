#include "XenRoll/PluginEditor.h"
#include "XenRoll/PluginProcessor.h"

namespace audio_plugin {
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p) {
    partialsVec partials = {};
    int partialsTotalCents = 5700;
    if (!p.params.get_tonesPartials().empty()) {
        partials = p.params.get_tonesPartials().begin()->second;
        partialsTotalCents = p.params.get_tonesPartials().begin()->first;
    }
    dissonanceMeter =
        std::make_shared<DissonanceMeter>(partials, partialsTotalCents, p.params.A4Freq,
                                          p.params.roughCompactFrac, p.params.dissonancePow);

    pitchMemory = std::make_shared<PitchMemory>(
        dissonanceMeter, processorRef.params.pitchMemoryTVvalForZeroHV,
        processorRef.params.pitchMemoryTVaddInfluence, processorRef.params.pitchMemoryTVminNonzero);

    pitchMemoryThreadPool = std::make_unique<juce::ThreadPool>(1);

    startTimer(timerMs);

    Theme::setTheme(processorRef.params.themeType);

    customLF = std::make_unique<CustomLookAndFeel>();
    juce::LookAndFeel::setDefaultLookAndFeel(customLF.get());

    tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 300);

    leftPanel = std::make_unique<LeftPanel>(octave_height_px, leftPanel_width_px, this,
                                            &(processorRef.params));
    leftViewport = std::make_unique<juce::Viewport>();
    leftViewport->setScrollBarsShown(false, false);
    leftViewport->setViewedComponent(leftPanel.get(), false);
    addAndMakeVisible(leftViewport.get());

    topPanel =
        std::make_unique<TopPanel>(bar_width_px, topPanel_height_px, this, &(processorRef.params));
    topViewport = std::make_unique<juce::Viewport>();
    topViewport->setScrollBarsShown(false, false);
    topViewport->setViewedComponent(topPanel.get(), false);
    addAndMakeVisible(topViewport.get());

    mainPanel =
        std::make_unique<MainPanel>(octave_height_px, bar_width_px, this, &(processorRef.params));
    mainViewport = std::make_unique<MainViewport>(leftViewport.get(), topViewport.get());
    mainViewport->setScrollBarsShown(true, true);
    mainViewport->setViewedComponent(mainPanel.get(), false);
    mainPanel->setPlayHeadTime(playHeadTime);
    addAndMakeVisible(mainViewport.get());

    helpPanel = std::make_unique<HelpPanel>();
    helpViewport = std::make_unique<HelpViewport>();
    helpViewport->setViewedComponent(helpPanel.get(), false);
    addAndMakeVisible(helpViewport.get());
    helpViewport->setVisible(false);

    settingsPanel = std::make_unique<SettingsPanel>(&(processorRef.params), this);
    addAndMakeVisible(settingsPanel.get());
    settingsPanel->setVisible(false);

    dissonancePanel = std::make_unique<DissonancePanel>(&(processorRef.params), dissonanceMeter);
    addAndMakeVisible(dissonancePanel.get());
    dissonancePanel->setVisible(false);

    pitchMemorySettingsPanel =
        std::make_unique<PitchMemorySettingsPanel>(&(processorRef.params), this);
    addAndMakeVisible(pitchMemorySettingsPanel.get());
    pitchMemorySettingsPanel->setVisible(false);

    settingsButton =
        std::make_unique<SVGButton>(BinaryData::Settings_svg, BinaryData::Settings_svgSize, false);
    addAndMakeVisible(settingsButton.get());

    helpButton = std::make_unique<SVGButton>(BinaryData::Help_svg, BinaryData::Help_svgSize, false);
    addAndMakeVisible(helpButton.get());

    dissonanceButton =
        std::make_unique<SVGButton>(BinaryData::Partials_svg, BinaryData::Partials_svgSize, false);
    addAndMakeVisible(dissonanceButton.get());

    pitchMemorySettingsButton = std::make_unique<SVGButton>(
        BinaryData::Brain_settings_svg, BinaryData::Brain_settings_svgSize, false);
    addAndMakeVisible(pitchMemorySettingsButton.get());

    settingsButton->onClick = [this]() {
        settingsPanel->setVisible(!settingsPanel->isVisible());

        helpViewport->setVisible(false);
        dissonancePanel->setVisible(false);
        pitchMemorySettingsPanel->setVisible(false);

        hideVelocityPanel();
    };
    helpButton->onClick = [this]() {
        helpViewport->setVisible(!helpViewport->isVisible());

        settingsPanel->setVisible(false);
        dissonancePanel->setVisible(false);
        pitchMemorySettingsPanel->setVisible(false);

        hideVelocityPanel();
    };
    dissonanceButton->onClick = [this]() {
        dissonancePanel->setVisible(!dissonancePanel->isVisible());

        helpViewport->setVisible(false);
        settingsPanel->setVisible(false);
        pitchMemorySettingsPanel->setVisible(false);

        hideVelocityPanel();
    };
    pitchMemorySettingsButton->onClick = [this]() {
        pitchMemorySettingsPanel->setVisible(!pitchMemorySettingsPanel->isVisible());

        settingsPanel->setVisible(false);
        helpViewport->setVisible(false);
        dissonancePanel->setVisible(false);

        hideVelocityPanel();
    };

    pitchMemoryButton =
        std::make_unique<SVGButton>(BinaryData::Brain_svg, BinaryData::Brain_svgSize, true,
                                    processorRef.params.showPitchesMemoryTraces,
                                    "Show pitch memory traces & notes' harmonicity");
    pitchMemoryButton->onClick = [this]() {
        processorRef.params.showPitchesMemoryTraces = !processorRef.params.showPitchesMemoryTraces;
        if (processorRef.params.showPitchesMemoryTraces) {
            updatePitchMemory();
        } else {
            mainPanel->repaint();
        }
    };
    addAndMakeVisible(pitchMemoryButton.get());

    keysHarmonicityButton = std::make_unique<SVGButton>(
        BinaryData::Keys_harmonicity_svg, BinaryData::Keys_harmonicity_svgSize, true,
        processorRef.params.showKeysHarmonicity,
        "Show harmonicity of keys (in the context of the end of the track) in an octave, an octave "
        "above and an octave below of all notes.");
    keysHarmonicityButton->onClick = [this]() {
        processorRef.params.showKeysHarmonicity = !processorRef.params.showKeysHarmonicity;
        if (processorRef.params.showKeysHarmonicity) {
            updatePitchMemory();
        } else {
            leftPanel->repaint();
        }
    };
    addAndMakeVisible(keysHarmonicityButton.get());

    ghostNotesKeysButton = std::make_unique<SVGButton>(
        BinaryData::Ghost_notes_keys_svg, BinaryData::Ghost_notes_keys_svgSize, true,
        processorRef.params.showGhostNotesKeys,
        "Show keys of \"ghost\" notes (notes from other instances of XenRoll).");
    ghostNotesKeysButton->onClick = [this]() {
        processorRef.params.showGhostNotesKeys = !processorRef.params.showGhostNotesKeys;
        mainPanel->remakeKeys();
        mainPanel->repaint();
    };
    addAndMakeVisible(ghostNotesKeysButton.get());

    instancesMenu =
        std::make_unique<InstancesMenu>(processorRef.params.channelIndex, &processorRef.params, this);
    addAndMakeVisible(instancesMenu.get());
    instancesMenu->setVisible(false);

    ghostNotesTabButton = std::make_unique<SVGButton>(
        BinaryData::Ghost_notes_tab_svg, BinaryData::Ghost_notes_tab_svgSize, false, false,
        "Choose instances of XenRoll from which you want to see notes");
    ghostNotesTabButton->onClick = [this]() {
        this->instancesMenu->setVisible(!this->instancesMenu->isVisible());
    };
    addAndMakeVisible(ghostNotesTabButton.get());

    numSubdivsLabel = std::make_unique<juce::Label>();
    numSubdivsLabel->setText("SUBDIVS", juce::dontSendNotification);
    juce::Font currentFont = numSubdivsLabel->getFont();
    currentFont.setHeight(Theme::big);
    numSubdivsLabel->setFont(currentFont);
    numSubdivsLabel->setColour(juce::Label::textColourId, Theme::brightest);
    addAndMakeVisible(numSubdivsLabel.get());
    numSubdivsInput = std::make_unique<IntegerInput>(processorRef.params.num_subdivs,
                                                     processorRef.params.min_num_subdivs,
                                                     processorRef.params.max_num_subdivs);
    numSubdivsInput->onValueChanged = [this](int newValue) {
        processorRef.params.num_subdivs = newValue;
        mainPanel->repaint();
    };
    addAndMakeVisible(numSubdivsInput.get());

    numBeatsLabel = std::make_unique<juce::Label>();
    numBeatsLabel->setText("BEATS", juce::dontSendNotification);
    numBeatsLabel->setFont(currentFont);
    numBeatsLabel->setColour(juce::Label::textColourId, Theme::brightest);
    addAndMakeVisible(numBeatsLabel.get());
    numBeatsInput = std::make_unique<IntegerInput>(processorRef.params.num_beats,
                                                   processorRef.params.min_num_beats,
                                                   processorRef.params.max_num_beats);
    numBeatsInput->onValueChanged = [this](int newValue) {
        processorRef.params.num_beats = newValue;
        mainPanel->repaint();
    };
    addAndMakeVisible(numBeatsInput.get());

    numBarsLabel = std::make_unique<juce::Label>();
    numBarsLabel->setText("BARS", juce::dontSendNotification);
    numBarsLabel->setFont(currentFont);
    numBarsLabel->setColour(juce::Label::textColourId, Theme::brightest);
    addAndMakeVisible(numBarsLabel.get());
    numBarsInput = std::make_unique<IntegerInput>(processorRef.params.get_num_bars(),
                                                  processorRef.params.min_num_bars,
                                                  processorRef.params.max_num_bars);
    numBarsInput->onValueChanged = [this](int newValue) {
        processorRef.params.set_num_bars(newValue);
        mainPanel->numBarsChanged();
        topPanel->numBarsChanged();
    };
    addAndMakeVisible(numBarsInput.get());

    midiChannelLabel = std::make_unique<juce::Label>();
    bool isActive = processorRef.getIsActive();
    juce::String midiChannelText = "MIDI CHANNEL: ";
    if (isActive) {
        midiChannelText += juce::String(processorRef.params.channelIndex + 1);
    } else {
        midiChannelText += "-";
    }
    midiChannelLabel->setText(midiChannelText, juce::dontSendNotification);
    currentFont.setHeight(Theme::medium);
    midiChannelLabel->setFont(currentFont);
    midiChannelLabel->setColour(juce::Label::textColourId, Theme::brightest);
    addAndMakeVisible(midiChannelLabel.get());

    camOnPlayHeadButton = std::make_unique<SVGButton>(
        BinaryData::Fix_cam_svg, BinaryData::Fix_cam_svgSize, true,
        processorRef.params.isCamFixedOnPlayHead, "Fix the camera to the playback head");
    camOnPlayHeadButton->onClick = [this]() {
        processorRef.params.isCamFixedOnPlayHead = !processorRef.params.isCamFixedOnPlayHead;
    };
    addAndMakeVisible(camOnPlayHeadButton.get());

    turnOnAllZonesButton = std::make_unique<SVGButton>(
        BinaryData::Zones_on_svg, BinaryData::Zones_on_svgSize, false, false, "Turn on all zones");
    turnOnAllZonesButton->onClick = [this]() {
        processorRef.params.zones.turnOnAllZones();
        zonesChanged();
        topPanel->repaint();
    };
    addAndMakeVisible(turnOnAllZonesButton.get());

    turnOffAllZonesButton =
        std::make_unique<SVGButton>(BinaryData::Zones_off_svg, BinaryData::Zones_off_svgSize, false,
                                    false, "Turn off all zones");
    turnOffAllZonesButton->onClick = [this]() {
        processorRef.params.zones.turnOffAllZones();
        zonesChanged();
        topPanel->repaint();
    };
    addAndMakeVisible(turnOffAllZonesButton.get());

    timeSnapButton =
        std::make_unique<SVGButton>(BinaryData::Snap_time_svg, BinaryData::Snap_time_svgSize, true,
                                    processorRef.params.timeSnap, "Snap notes horizontally");
    timeSnapButton->onClick = [this]() {
        processorRef.params.timeSnap = !processorRef.params.timeSnap;
    };
    addAndMakeVisible(timeSnapButton.get());

    keySnapButton =
        std::make_unique<SVGButton>(BinaryData::Snap_keys_svg, BinaryData::Snap_keys_svgSize, true,
                                    processorRef.params.keySnap, "Snap notes vertically");
    keySnapButton->onClick = [this]() {
        processorRef.params.keySnap = !processorRef.params.keySnap;
    };
    addAndMakeVisible(keySnapButton.get());

    importButton = std::make_unique<SVGButton>(BinaryData::Import_svg, BinaryData::Import_svgSize,
                                               false, false, "Import");
    importButton->onClick = [this]() {
        auto *alert = new juce::AlertWindow("Import notes", "Choose an option",
                                            juce::MessageBoxIconType::NoIcon);

        alert->setUsingNativeTitleBar(true);
        alert->addButton(".mid OR .mid + .scl (no bends info)", 1);
        alert->addButton(".notes", 2);

        alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int result) {
                                   if (result == 1) {
                                       importMidiSclFiles();
                                   } else if (result == 2) {
                                       importNotesFile();
                                   }
                                   alert->exitModalState(result);
                                   alert->setVisible(false);
                               }),
                               true);
    };
    addAndMakeVisible(importButton.get());

    exportButton = std::make_unique<SVGButton>(BinaryData::Export_svg, BinaryData::Export_svgSize,
                                               false, false, "Export");
    exportButton->onClick = [this]() {
        auto *alert = new juce::AlertWindow("Export notes", "Choose an option",
                                            juce::MessageBoxIconType::NoIcon);

        alert->setUsingNativeTitleBar(true);
        alert->addButton(".mid OR .mid + .scl (no bends info)", 1);
        alert->addButton(".notes", 2);

        alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int result) {
                                   if (result == 1) {
                                       exportMidiSclFiles();
                                   } else if (result == 2) {
                                       exportNotesFile();
                                   }
                                   alert->exitModalState(result);
                                   alert->setVisible(false);
                               }),
                               true);
    };
    addAndMakeVisible(exportButton.get());

    popup = std::make_unique<PopupMessage>(3000, 0.7f);
    addChildComponent(popup.get());

    velocityPanel = std::make_unique<VelocityPanel>(
        [this]() { mainPanel->setVelocitiesOfSelectedNotes(this->velocityPanel->getVelocity()); });
    addAndMakeVisible(velocityPanel.get());
    velocityPanel->setVisible(false);

    importFileChooser = std::make_unique<juce::FileChooser>(
        "Import notes", juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.mid;*.midi;*.scl;*.notes");
    exportFileChooser = std::make_unique<juce::FileChooser>(
        "Export notes", juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.mid;*.midi;*.notes");

    setSize(processorRef.params.editorWidth, processorRef.params.editorHeight);
    setResizable(true, true);
    setResizeLimits(processorRef.params.min_editorWidth, processorRef.params.min_editorHeight,
                    processorRef.params.max_editorWidth, processorRef.params.max_editorHeight);
    setWantsKeyboardFocus(false);

    if (processorRef.params.showPitchesMemoryTraces) {
        updatePitchMemory();
    }

    if (!isActive) {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, "ERROR",
            juce::String("One of two things happened:") +
                "1. You have exceeded the limit of instances of this plugin (16).\n" +
                "FIX: Delete this instance, it won't work.\n\n" +
                "2. There was corruption of the data needed to synchronize the instances, or one "
                "of the instances crashed last time.\n" +
                "FIX: Close your DAW and open it again (If this does not help, then restart your "
                "PC).",
            "OK");
    }
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    processorRef.params.editorWidth = getWidth();
    processorRef.params.editorHeight = getHeight();
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g) {
    // (Our component is opaque, so we must completely fill the background with a
    // solid colour)
    g.fillAll(Theme::darker);

    const int width = getWidth();
    const int height = getHeight();
    const int leftView_height_px =
        height - topPanel_height_px - 2 * bottom_gap_height_px - bottom_height_px - slider_width_px;
    const int bottom_y =
        topPanel_height_px + leftView_height_px + slider_width_px + bottom_gap_height_px;

    juce::String text = midiChannelLabel->getText();
    juce::Font font = midiChannelLabel->getFont();
    int textWidth = juce::roundToInt(getTextWidth(text, font));
    int bottom_x_pos = width - 15 * 4 - textWidth - 2 * bottom_height_px;

    // Draw vertical separator line
    g.setColour(Theme::darkest);
    g.drawLine(bottom_x_pos, bottom_y, bottom_x_pos, bottom_y + bottom_height_px, Theme::wider);
}

void AudioPluginAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();
    const int width = getWidth();
    const int height = getHeight();

    const int leftView_height_px =
        height - topPanel_height_px - 2 * bottom_gap_height_px - bottom_height_px - slider_width_px;
    const int topView_width_px = width - leftPanel_width_px - slider_width_px;
    const int bottom_y =
        topPanel_height_px + leftView_height_px + slider_width_px + bottom_gap_height_px;

    const juce::Rectangle<int> allBesidesBottomRect =
        juce::Rectangle<int>(0, 0, width, height - bottom_height_px - 2 * bottom_gap_height_px);

    leftViewport->setSize(leftPanel_width_px, leftView_height_px);
    topViewport->setSize(topView_width_px, topPanel_height_px);
    mainViewport->setSize(topView_width_px + slider_width_px, leftView_height_px + slider_width_px);

    leftViewport->setBounds(0, topPanel_height_px, leftPanel_width_px, leftView_height_px);
    topViewport->setBounds(leftPanel_width_px, 0, topView_width_px, topPanel_height_px);
    mainViewport->setBounds(leftPanel_width_px, topPanel_height_px,
                            topView_width_px + slider_width_px,
                            leftView_height_px + slider_width_px);

    popup->setBounds(leftPanel_width_px + (topView_width_px - popup_width_px) / 2,
                     20 + topPanel_height_px, popup_width_px, popup_height_px);

    helpViewport->setBounds(allBesidesBottomRect);
    helpPanel->setSize(width - slider_width_px, 1000);

    settingsPanel->setBounds(allBesidesBottomRect);
    dissonancePanel->setBounds(allBesidesBottomRect);
    pitchMemorySettingsPanel->setBounds(allBesidesBottomRect);

    int top_x_pos = top_x;
    camOnPlayHeadButton->setBounds(top_x_pos, top_y, top_height_px, top_height_px);
    top_x_pos += top_height_px + 15;
    turnOnAllZonesButton->setBounds(top_x_pos, top_y, top_height_px, top_height_px);
    top_x_pos += top_height_px + 15;
    turnOffAllZonesButton->setBounds(top_x_pos, top_y, top_height_px, top_height_px);

    int bottom_x_pos = bottom_x;
    settingsButton->setBounds(bottom_x_pos, bottom_y, bottom_height_px, bottom_height_px);
    bottom_x_pos += bottom_height_px + 15;
    helpButton->setBounds(bottom_x_pos, bottom_y, bottom_height_px, bottom_height_px);

    juce::String text = numBarsLabel->getText();
    juce::Font font = numBarsLabel->getFont();
    int textWidth = juce::roundToInt(getTextWidth(text, font));
    bottom_x_pos += bottom_height_px + 15;
    numBarsLabel->setBounds(bottom_x_pos, bottom_y, textWidth, bottom_height_px);
    bottom_x_pos += textWidth + 5;
    numBarsInput->setBounds(bottom_x_pos, bottom_y, 50, bottom_height_px);

    text = numBeatsLabel->getText();
    font = numBeatsLabel->getFont();
    textWidth = juce::roundToInt(getTextWidth(text, font));
    bottom_x_pos += 50 + 10;
    numBeatsLabel->setBounds(bottom_x_pos, bottom_y, textWidth, bottom_height_px);
    bottom_x_pos += textWidth + 5;
    numBeatsInput->setBounds(bottom_x_pos, bottom_y, 40, bottom_height_px);

    text = numSubdivsLabel->getText();
    font = numSubdivsLabel->getFont();
    textWidth = juce::roundToInt(getTextWidth(text, font));
    bottom_x_pos += 40 + 10;
    numSubdivsLabel->setBounds(bottom_x_pos, bottom_y, textWidth, bottom_height_px);
    bottom_x_pos += textWidth + 5;
    numSubdivsInput->setBounds(bottom_x_pos, bottom_y, 40, bottom_height_px);

    bottom_x_pos += 40 + 15;
    timeSnapButton->setBounds(bottom_x_pos, bottom_y, bottom_height_px, bottom_height_px);
    bottom_x_pos += bottom_height_px + 15;
    keySnapButton->setBounds(bottom_x_pos, bottom_y, bottom_height_px, bottom_height_px);
    bottom_x_pos += bottom_height_px + 15;
    importButton->setBounds(bottom_x_pos, bottom_y, bottom_height_px, bottom_height_px);
    bottom_x_pos += bottom_height_px + 15;
    exportButton->setBounds(bottom_x_pos, bottom_y, bottom_height_px, bottom_height_px);

    text = midiChannelLabel->getText();
    font = midiChannelLabel->getFont();
    textWidth = juce::roundToInt(getTextWidth(text, font));
    bottom_x_pos = width - 15 - textWidth;
    midiChannelLabel->setBounds(width - 15 - textWidth, bottom_y, textWidth, bottom_height_px);
    bottom_x_pos -= 15 + bottom_height_px;
    ghostNotesKeysButton->setBounds(bottom_x_pos, bottom_y, bottom_height_px, bottom_height_px);
    bottom_x_pos -= 15 + bottom_height_px;
    ghostNotesTabButton->setBounds(bottom_x_pos, bottom_y, bottom_height_px, bottom_height_px);
    bottom_x_pos -= 30 + bottom_height_px;

    instancesMenu->setBounds(bottom_x_pos - 20, bottom_y - instancesMenu->getHeight() - 10,
                             instancesMenu->getWidth(), instancesMenu->getHeight());

    dissonanceButton->setBounds(bottom_x_pos, bottom_y, bottom_height_px, bottom_height_px);
    bottom_x_pos -= 15 + bottom_height_px;
    pitchMemorySettingsButton->setBounds(bottom_x_pos, bottom_y, bottom_height_px,
                                         bottom_height_px);
    bottom_x_pos -= 15 + bottom_height_px;
    pitchMemoryButton->setBounds(bottom_x_pos, bottom_y, bottom_height_px, bottom_height_px);
    bottom_x_pos -= 15 + bottom_height_px;
    keysHarmonicityButton->setBounds(bottom_x_pos, bottom_y, bottom_height_px, bottom_height_px);

    velocityPanel->setBounds(leftPanel_width_px + (topView_width_px - velocity_width_px) / 2,
                             topPanel_height_px + leftView_height_px - velocity_height_px -
                                 velocity_bot_gap_px,
                             velocity_width_px, velocity_height_px);

    repaint();
}

std::optional<std::vector<int>> parseSclFile(const juce::File &file) {
    juce::FileInputStream inputStream(file);
    if (!inputStream.openedOk()) {
        return std::nullopt;
    }

    std::vector<int> scale;
    int currentLine = 0;
    int pitchLinesRead = 0;
    bool gotDescription = false;
    bool gotNoteCount = false;
    int noteCount = 0;

    while (!inputStream.isExhausted()) {
        auto line = inputStream.readNextLine().trim();
        currentLine++;

        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWithChar('!')) {
            continue;
        }

        // First non-comment line is description
        if (!gotDescription) {
            gotDescription = true;
            continue;
        }

        // Second non-comment line is note count
        if (!gotNoteCount) {
            noteCount = line.getIntValue();

            if (noteCount < 0) {
                DBG("Invalid note count: " + line);
                return std::nullopt;
            }

            gotNoteCount = true;
            continue;
        }

        // Parse pitch values
        if (pitchLinesRead < noteCount) {
            // Remove everything after first whitespace
            auto valueStr = line.upToFirstOccurrenceOf(" ", false, false)
                                .upToFirstOccurrenceOf("\t", false, false);

            if (valueStr.containsChar('.')) {
                // Cents value
                double cents = valueStr.getDoubleValue();
                if (cents < 0) {
                    DBG("Invalid cents value: " + valueStr);
                    return std::nullopt;
                }
                scale.push_back(int(round(cents)));
            } else if (valueStr.containsChar('/')) {
                // Ratio
                auto numeratorStr = valueStr.upToFirstOccurrenceOf("/", false, false);
                auto denominatorStr = valueStr.fromFirstOccurrenceOf("/", false, false);

                int numerator = numeratorStr.getIntValue();
                int denominator = denominatorStr.getIntValue();

                if (numerator <= 0 || denominator <= 0) {
                    DBG("Invalid ratio: " + valueStr);
                    return std::nullopt;
                }

                double ratio = static_cast<double>(numerator) / denominator;
                double cents = 1200.0 * log2(ratio);
                scale.push_back(int(round(cents)));
            } else {
                // Integer (treated as ratio)
                int value = valueStr.getIntValue();
                if (value <= 0) {
                    DBG("Invalid integer value: " + valueStr);
                    return std::nullopt;
                }
                double cents = 1200.0 * log2(static_cast<double>(value));
                scale.push_back(int(round(cents)));
            }

            pitchLinesRead++;
        }
    }

    // Validation
    if (scale.size() != noteCount) {
        DBG("Note count mismatch. Expected: " + juce::String(noteCount) +
            ", Got: " + juce::String(scale.size()));
        return std::nullopt;
    }

    return scale;
}

void AudioPluginAudioProcessorEditor::importMidiSclFiles() {
    importFileChooser.get()->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles |
            juce::FileBrowserComponent::canSelectMultipleItems,
        [this](const juce::FileChooser &fc) {
            auto files = fc.getResults();
            if (files.size() == 0)
                return;

            juce::File midiFile;
            juce::File sclFile;
            bool hasMidi = false;
            bool hasScl = false;

            for (const auto &file : files) {
                if (file.hasFileExtension(".mid") || file.hasFileExtension(".midi")) {
                    if (hasMidi) {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon, "Error",
                            "Please select only one MIDI file", "OK");
                        return;
                    }
                    midiFile = file;
                    hasMidi = true;
                } else if (file.hasFileExtension(".scl")) {
                    if (hasScl) {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon, "Error",
                            "Please select only one .scl file", "OK");
                        return;
                    }
                    sclFile = file;
                    hasScl = true;
                }
            }

            if (!hasMidi) {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error",
                                                       "Please select at least one MIDI file",
                                                       "OK");
                return;
            }

            std::vector<int> sclScale;
            if (hasScl) {
                auto sclScaleOpt = parseSclFile(sclFile);
                if (!sclScaleOpt) {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error",
                                                           "Failed to parse .scl file", "OK");
                    return;
                }
                sclScale = sclScaleOpt.value();
                sclScale.insert(sclScale.begin(), 0);
            }

            juce::MidiFile midi;
            juce::FileInputStream inputStream(midiFile);
            std::vector<Note> notes;

            if (!inputStream.openedOk() || !midi.readFrom(inputStream)) {
                return;
            }

            // Get ticks per quarter note from the MIDI file header
            double ticksPerQuarterNote = midi.getTimeFormat();
            if (ticksPerQuarterNote < 0) {
                // If negative, it's SMPTE format which we don't handle here
                ticksPerQuarterNote = 960.0; // fallback to default
            }

            // Convert MIDI tracks to single sequence of events
            // midi.convertTimestampTicksToSeconds();

            int totalBars = 0;
            int beatsPerBar = 4;
            int subdivisionsPerBeat = 4;
            double bpm = 120.0;
            int numerator = 4, denominator = 4;
            double maxBarPosition = 0.0;

            // Find tempo and time signature
            for (int i = 0; i < midi.getNumTracks(); i++) {
                const auto *track = midi.getTrack(i);
                for (int j = 0; j < track->getNumEvents(); j++) {
                    const auto &msg = track->getEventPointer(j)->message;
                    if (msg.isTempoMetaEvent()) {
                        bpm = msg.getTempoSecondsPerQuarterNote() > 0
                                  ? 60.0 / msg.getTempoSecondsPerQuarterNote()
                                  : 120.0;
                    } else if (msg.isTimeSignatureMetaEvent()) {
                        msg.getTimeSignatureInfo(numerator, denominator);
                        beatsPerBar = numerator;
                        subdivisionsPerBeat = 4 * (4 / denominator);
                    }
                    // Track the latest note end time to calculate total bars
                    if (msg.isNoteOff()) {
                        maxBarPosition = std::max(maxBarPosition, msg.getTimeStamp());
                    }
                }
            }

            // Calculate ticks per bar
            const double ticksPerBar = ticksPerQuarterNote * 4.0 * numerator / denominator;
            totalBars = static_cast<int>(std::ceil(maxBarPosition / ticksPerBar));

            // Process all note events
            for (int i = 0; i < midi.getNumTracks(); i++) {
                const auto *track = midi.getTrack(i);
                for (int j = 0; j < track->getNumEvents(); j++) {
                    const auto *event = track->getEventPointer(j);
                    const auto &msg = event->message;

                    if (msg.isNoteOn()) {
                        // Find matching note-off
                        const auto *noteOffEvent = event->noteOffObject;
                        if (!noteOffEvent)
                            continue;

                        // Calculate timing in bars
                        const double startTimeTicks = msg.getTimeStamp();
                        const double endTimeTicks = noteOffEvent->message.getTimeStamp();

                        const double startTimeBars = startTimeTicks / ticksPerBar;
                        const double durationBars = (endTimeTicks - startTimeTicks) / ticksPerBar;

                        // Convert MIDI note to octave and cents
                        const int midiNote = msg.getNoteNumber();
                        int octave, cents;
                        if (hasScl) {
                            int n = int(sclScale.size()) - 1;
                            int totalCents = sclScale[n] * (midiNote / n) + sclScale[midiNote % n];
                            octave = totalCents / 1200;
                            if (octave >= processorRef.params.num_octaves) {
                                juce::AlertWindow::showMessageBoxAsync(
                                    juce::AlertWindow::WarningIcon, "Error",
                                    "There is a too high pitched note", "OK");
                                return;
                            }
                            cents = totalCents % 1200;
                        } else {
                            octave = (midiNote / 12);
                            cents = (midiNote % 12) * 100; // 0-1100 in 100-cent steps
                        }

                        // Create Note struct
                        notes.push_back({octave, cents, static_cast<float>(startTimeBars), false,
                                         static_cast<float>(durationBars), msg.getFloatVelocity()});
                    }
                }
            }

            this->numBarsInput.get()->onValueChanged(totalBars);
            this->numBeatsInput.get()->onValueChanged(beatsPerBar);
            this->numSubdivsInput.get()->onValueChanged(subdivisionsPerBeat);

            this->numBarsInput.get()->setValue(totalBars);
            this->numBeatsInput.get()->setValue(beatsPerBar);
            this->numSubdivsInput.get()->setValue(subdivisionsPerBeat);

            this->mainPanel->updateNotes(notes);
            this->updateNotes(notes);

            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Imported!",
                                                   "BPM of imported track: " + juce::String(bpm),
                                                   "OK");
        });
}

void AudioPluginAudioProcessorEditor::exportMidiSclFiles() {
    std::vector<int> keysFromNotes;
    keysFromNotes.push_back(0);
    const std::vector<Note> &notes = getNotes();
    std::vector<int> keysIndexes(notes.size());

    for (int i = 0; i < notes.size(); ++i) {
        const Note &note = notes[i];
        int totalCents = note.octave * 1200 + note.cents;
        auto it = std::find(keysFromNotes.begin(), keysFromNotes.end(), totalCents);
        if (it == keysFromNotes.end()) {
            auto itit = std::lower_bound(keysFromNotes.begin(), keysFromNotes.end(), totalCents);
            keysFromNotes.insert(itit, totalCents);
        }
    }
    for (int i = 0; i < notes.size(); ++i) {
        const Note &note = notes[i];
        int totalCents = note.octave * 1200 + note.cents;
        auto it = std::find(keysFromNotes.begin(), keysFromNotes.end(), totalCents);
        keysIndexes[i] = int(std::distance(keysFromNotes.begin(), it));
    }

    exportFileChooser.get()->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, keysFromNotes, notes, keysIndexes](const juce::FileChooser &fc) {
            juce::File midiFile = fc.getResult();
            if (midiFile == juce::File{})
                return;
            midiFile.deleteFile();
            juce::File sclFile = midiFile.withFileExtension(".scl");
            sclFile.deleteFile();

            // Exporting .scl file
            juce::FileOutputStream outputStreamScl(sclFile);
            if (!outputStreamScl.openedOk()) {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon, "Error",
                    "Failed to create file: " + sclFile.getFullPathName(), "OK");
                return;
            }

            outputStreamScl.writeText("! " + sclFile.getFileName() + "\n", false, false, nullptr);
            outputStreamScl.writeText("Auto generated by XenRoll\n", false, false, nullptr);
            outputStreamScl.writeText(juce::String(keysFromNotes.size() - 1) + "\n", false, false,
                                      nullptr);
            for (int i = 1; i < keysFromNotes.size(); ++i) {
                outputStreamScl.writeText(juce::String(keysFromNotes[i]) + ".0\n", false, false,
                                          nullptr);
            }
            outputStreamScl.flush();

            // Exporting .mid file
            juce::MidiFile midiFilemidi;
            juce::MidiMessageSequence track;

            const int ticksPerQuarterNote = 960;
            midiFilemidi.setTicksPerQuarterNote(ticksPerQuarterNote);

            auto bpmNumDenom = this->processorRef.getBpmNumDenom();
            track.addEvent(juce::MidiMessage::timeSignatureMetaEvent(std::get<1>(bpmNumDenom),
                                                                     std::get<2>(bpmNumDenom)));

            const float microsecondsPerQuarterNote = 60000000.0f / std::get<0>(bpmNumDenom);
            track.addEvent(
                juce::MidiMessage::tempoMetaEvent(static_cast<int>(microsecondsPerQuarterNote)));

            const float ticksPerBar =
                ticksPerQuarterNote * 4.0f *
                (std::get<1>(bpmNumDenom) / static_cast<float>(std::get<2>(bpmNumDenom)));

            for (int i = 0; i < notes.size(); i++) {
                const auto &note = notes[i];

                const double startTimeTicks = note.time * ticksPerBar;
                const double durationTicks = note.duration * ticksPerBar;
                const double endTimeTicks = startTimeTicks + durationTicks;

                const int midiNoteNumber = keysIndexes[i];
                const float velocity = note.velocity * 127.0f;

                track.addEvent(juce::MidiMessage::noteOn(1, midiNoteNumber, juce::uint8(velocity))
                                   .withTimeStamp(startTimeTicks));
                track.addEvent(
                    juce::MidiMessage::noteOff(1, midiNoteNumber).withTimeStamp(endTimeTicks));
            }

            midiFilemidi.addTrack(track);

            juce::FileOutputStream outputStreamMidi(midiFile);
            if (!outputStreamMidi.openedOk()) {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon, "Error",
                    "Failed to create file: " + midiFile.getFullPathName(), "OK");
                return;
            }

            midiFilemidi.writeTo(outputStreamMidi);
            outputStreamMidi.flush();
        });
}

void AudioPluginAudioProcessorEditor::exportNotesFile() {
    const std::vector<Note> &notes = getNotes();

    exportFileChooser.get()->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, notes](const juce::FileChooser &fc) {
            juce::File notesFile = fc.getResult();
            if (notesFile == juce::File{})
                return;
            notesFile.deleteFile();

            juce::FileOutputStream outputStream(notesFile);
            if (!outputStream.openedOk()) {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon, "Error",
                    "Failed to create file: " + notesFile.getFullPathName(), "OK");
                return;
            }

            // write time info
            outputStream.writeInt(this->processorRef.params.get_num_bars());
            outputStream.writeInt(this->processorRef.params.num_beats);
            outputStream.writeInt(this->processorRef.params.num_subdivs);
            outputStream.writeFloat(std::get<0>(this->processorRef.getBpmNumDenom()));

            // write notes
            outputStream.writeInt(notes.size());
            for (const Note &note : notes) {
                outputStream.writeInt(note.octave);
                outputStream.writeInt(note.cents);
                outputStream.writeFloat(note.time);
                outputStream.writeFloat(note.duration);
                outputStream.writeFloat(note.velocity);
                outputStream.writeInt(note.bend);
            }
            outputStream.flush();
        });
}

void AudioPluginAudioProcessorEditor::importNotesFile() {
    importFileChooser.get()->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser &fc) {
            juce::File notesFile = fc.getResult();
            if (notesFile == juce::File{})
                return;

            juce::FileInputStream inputStream(notesFile);
            if (!inputStream.openedOk()) {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon, "Error",
                    "Failed to open file: " + notesFile.getFullPathName(), "OK");
                return;
            }

            // read time info
            const int numBars = inputStream.readInt();
            const int numBeats = inputStream.readInt();
            const int numSubdivs = inputStream.readInt();
            const int bpm = inputStream.readFloat();

            const int numNotes = inputStream.readInt();
            std::vector<Note> notes(numNotes);
            for (int i = 0; i < numNotes; ++i) {
                notes[i].octave = inputStream.readInt();
                notes[i].cents = inputStream.readInt();
                notes[i].time = inputStream.readFloat();
                notes[i].duration = inputStream.readFloat();
                notes[i].velocity = inputStream.readFloat();
                notes[i].bend = inputStream.readInt();
            }

            this->numBarsInput.get()->onValueChanged(numBars);
            this->numBeatsInput.get()->onValueChanged(numBeats);
            this->numSubdivsInput.get()->onValueChanged(numSubdivs);

            this->numBarsInput.get()->setValue(numBars);
            this->numBeatsInput.get()->setValue(numBeats);
            this->numSubdivsInput.get()->setValue(numSubdivs);

            this->mainPanel->updateNotes(notes);
            this->updateNotes(notes);

            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Imported!",
                                                   "BPM of imported track: " + juce::String(bpm),
                                                   "OK");
        });
}
} // namespace audio_plugin