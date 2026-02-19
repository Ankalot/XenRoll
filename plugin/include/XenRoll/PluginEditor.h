#pragma once

#include "BinaryData.h"
#include "DissonanceMeter.h"
#include "DissonancePanel.h"
#include "DragAndDropPopup.h"
#include "EditRatiosMarksMenu.h"
#include "GenNewKeysMenu.h"
#include "HelpPanel.h"
#include "InstancesMenu.h"
#include "IntegerInput.h"
#include "LeftPanel.h"
#include "MainPanel.h"
#include "MoreToolsMenu.h"
#include "Note.h"
#include "PitchMemory.h"
#include "PitchMemorySettingsPanel.h"
#include "PluginProcessor.h"
#include "PopupMessage.h"
#include "SVGButton.h"
#include "SettingsPanel.h"
#include "Theme.h"
#include "TopPanel.h"
#include "VelocityPanel.h"
#include "VocalToNotesMenu.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace audio_plugin {
class CustomLookAndFeel : public juce::LookAndFeel_V4 {
  public:
    CustomLookAndFeel() {
        cambriaMathTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::CambriaMath_ttf, BinaryData::CambriaMath_ttfSize);
        setDefaultSansSerifTypeface(cambriaMathTypeface);
        updateColors();
    }

    void updateColors();

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font &) override {
        return cambriaMathTypeface;
    }

    void drawTooltip(juce::Graphics &g, const juce::String &text, int width, int height) override {
        juce::Rectangle<int> bounds(width, height);
        auto cornerSize = 5.0f;

        g.setColour(findColour(juce::TooltipWindow::backgroundColourId));
        g.fillRoundedRectangle(bounds.toFloat(), cornerSize);

        g.setColour(findColour(juce::TooltipWindow::outlineColourId));
        g.drawRoundedRectangle(bounds.toFloat(), cornerSize, Theme::narrow);

        auto tl = layoutTooltipText(text);
        tl.draw(g, {static_cast<float>(width), static_cast<float>(height)});
    }

    juce::TextLayout layoutTooltipText(const juce::String &text) {
        const int maxToolTipWidth = 400;

        juce::AttributedString s;
        s.setJustification(juce::Justification::centred);
        s.append(text, juce::FontOptions(Theme::small_, juce::Font::bold),
                 findColour(juce::TooltipWindow::textColourId));

        juce::TextLayout tl;
        tl.createLayoutWithBalancedLineLengths(s, (float)maxToolTipWidth);
        return tl;
    }

    juce::Rectangle<int> getTooltipBounds(const juce::String &tipText, juce::Point<int> screenPos,
                                          juce::Rectangle<int> parentArea) override {
        auto tl = layoutTooltipText(tipText);

        auto w = (int)(tl.getWidth() + 14.0f);
        auto h = (int)(tl.getHeight() + 6.0f);

        return juce::Rectangle<int>(screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12)
                                                                          : screenPos.x + 24,
                                    screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6)
                                                                          : screenPos.y + 6,
                                    w, h)
            .constrainedWithin(parentArea);
    }

    juce::Label *createSliderTextBox(juce::Slider &slider) override {
        auto *label = LookAndFeel_V4::createSliderTextBox(slider);
        label->setFont(label->getFont().withHeight(Theme::medium));
        return label;
    }

    juce::Font getComboBoxFont(juce::ComboBox &c) override {
        return LookAndFeel_V4::getComboBoxFont(c).withHeight(Theme::medium);
    }

    juce::Font getPopupMenuFont() override {
        return LookAndFeel_V4::getPopupMenuFont().withHeight(Theme::medium);
    }

  private:
    juce::Typeface::Ptr cambriaMathTypeface;
    const int padding_x = 6;
    const int padding_y = 2;
};

class SmallLookAndFeel : public juce::LookAndFeel_V4 {
  public:
    SmallLookAndFeel() {
        cambriaMathTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::CambriaMath_ttf, BinaryData::CambriaMath_ttfSize);
        setDefaultSansSerifTypeface(cambriaMathTypeface);
        updateColors();
    }

    void updateColors();

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font &) override {
        return cambriaMathTypeface;
    }

    juce::Label *createSliderTextBox(juce::Slider &slider) override {
        auto *label = LookAndFeel_V4::createSliderTextBox(slider);
        label->setFont(label->getFont().withHeight(Theme::small_));
        return label;
    }

    juce::Font getComboBoxFont(juce::ComboBox &c) override {
        return LookAndFeel_V4::getComboBoxFont(c).withHeight(Theme::small_);
    }

    juce::Font getPopupMenuFont() override {
        return LookAndFeel_V4::getPopupMenuFont().withHeight(Theme::small_);
    }

    void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                              const juce::Colour &backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

  private:
    juce::Typeface::Ptr cambriaMathTypeface;
};

class MainViewport : public juce::Viewport {
  public:
    MainViewport(juce::Viewport *leftViewport, juce::Viewport *topViewport)
        : leftViewport(leftViewport), topViewport(topViewport) {
        updateColors();
    }

    /**
     * @brief Set callback function to update viewport size
     */
    void setUpdateCallback(std::function<void()> callback) { updateCallback = callback; }

    /**
     * @brief Update scrollbar colors to match current theme
     */
    void updateColors() {
        getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, Theme::bright);
        getHorizontalScrollBar().setColour(juce::ScrollBar::thumbColourId, Theme::bright);
    }

    /**
     * @brief Center the viewport on the playhead position
     * @param playHeadTime Current playhead time in beats
     * @param bar_width_px Width of one bar in pixels
     */
    void setCamOnTime(float playHeadTime, int bar_width_px) {
        auto *viewedComponent = getViewedComponent();
        if (viewedComponent == nullptr)
            return;

        int viewWidth = getViewArea().getWidth();
        int targetX = juce::roundToInt(playHeadTime * bar_width_px - (viewWidth / 2.0f));

        auto maxX = juce::jmax(0, viewedComponent->getWidth() - viewWidth);
        targetX = juce::jlimit(0, maxX, targetX);

        setViewPosition(targetX, getViewPositionY());
    }

    bool isVerticalScrollBarVisible() const {
        auto *viewedComp = getViewedComponent();
        if (viewedComp == nullptr)
            return false;
        return viewedComp->getHeight() > getViewHeight();
    }

    bool isHorizontalScrollBarVisible() const {
        auto *viewedComp = getViewedComponent();
        if (viewedComp == nullptr)
            return false;
        return viewedComp->getWidth() > getViewWidth();
    }

  private:
    juce::Viewport *leftViewport;
    juce::Viewport *topViewport;
    std::function<void()> updateCallback;
    bool isUpdating = false; ///< Just in case, against infinite recursion

    /**
     * @brief Called when visible area changes, syncs with linked viewports
     * @param newVisibleArea New visible area rectangle
     */
    void visibleAreaChanged(const juce::Rectangle<int> &newVisibleArea) override {
        if (isUpdating) {
            return;
        }
        isUpdating = true;
        Viewport::visibleAreaChanged(newVisibleArea);
        leftViewport->setViewPosition(leftViewport->getViewPositionX(), newVisibleArea.getY());
        topViewport->setViewPosition(newVisibleArea.getX(), topViewport->getViewPositionY());

        // Update mainViewport size based on scrollbar visibility
        if (updateCallback != nullptr) {
            updateCallback();
        }
        isUpdating = false;
    }
};

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::FileDragAndDropTarget,
                                        private juce::Timer {
  public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &);
    ~AudioPluginAudioProcessorEditor() override;

    void paint(juce::Graphics &) override;
    void resized() override;

    void repaintTopPanel() { topPanel->repaint(); }

    void changeOctaveHeightPx(float new_octave_height_px) {
        octave_height_px = new_octave_height_px;
        leftPanel->changeOctaveHeightPx(octave_height_px);
    }

    void changeBarWidthPx(float new_bar_width_px) {
        bar_width_px = new_bar_width_px;
        topPanel->changeBarWidthPx(bar_width_px);
    }

    void updateKeys(std::set<int> keys) {
        leftPanel->updateKeys(keys);
        processorRef.updateKeys(keys);
    }

    void updateMainViewportSize();

    /**
     * @brief Show a popup message
     * @param message Message text to display
     */
    void showMessage(const juce::String &message) { popup->showMessage(message); }

    void remakeKeys() {
        mainPanel->remakeKeys();
        mainPanel->repaint();
    }

    /**
     * @brief Called when zones configuration changes
     */
    void zonesChanged() {
        remakeKeys();
        updatePitchMemory();
    }

    /**
     * @brief Called when ratios marks settings change
     */
    void editRatiosMarksMenuChanged() { mainPanel->updateRatiosMarks(); }

    void quantizeSelectedNotes() { mainPanel->quantizeSelectedNotes(); }

    void randomizeSelectedNotesTiming() { mainPanel->randomizeSelectedNotesTiming(); }

    void randomizeSelectedNotesVelocity() { mainPanel->randomizeSelectedNotesVelocity(); }

    /**
     * @brief Hide the current popup message
     */
    void hideMessage() { popup->timerCallback(); }

    /**
     * @brief Get the text from the current popup message
     * @return Message string
     */
    const juce::String &getTextFromMessage() { return popup->getText(); }

    /**
     * @brief Get BPM, numerator and denominator
     * @return Tuple of (BPM, numerator, denominator)
     */
    std::tuple<float, int, int> getBpmNumDenom() { return processorRef.getBpmNumDenom(); }

    /**
     * @brief Update the notes and recalculate pitch memory
     * @param new_notes New notes vector
     */
    void updateNotes(const std::vector<Note> &new_notes) {
        processorRef.updateNotes(new_notes);
        updatePitchMemory();
    }

    /**
     * @brief Update pitch memory traces and harmonicity
     */
    void updatePitchMemory();

    /**
     * @brief Re-prepare notes for playback
     */
    void rePrepareNotes() { processorRef.rePrepareNotes(); }

    void setManuallyPlayedNotesTotalCents(const std::set<int> &manuallyPlayedNotesTotalCents) {
        processorRef.setManuallyPlayedNotesTotalCents(manuallyPlayedNotesTotalCents);
    }

    const std::vector<Note> &getNotes() { return processorRef.getNotes(); }

    void showVelocityPanel(float initVel) {
        velocityPanel->setVelocity(initVel);
        velocityPanel->setVisible(true);
    }

    void hideVelocityPanel() { velocityPanel->setVisible(false); }

    void updateGhostNotes() {
        const auto ghostNotes = processorRef.getOtherInstancesNotes();
        mainPanel->updateGhostNotes(ghostNotes);
    }

    /**
     * @brief Timer callback for UI updates
     */
    void timerCallback();

    /**
     * @brief Bring keyboard focus back to main panel
     */
    void bringBackKeyboardFocus() { mainPanel->grabKeyboardFocus(); }

    /**
     * @brief Called when A4 frequency changes
     * @note Re-prepairs notes
     */
    void A4freqChanged() {
        dissonanceMeter->setA4freq(processorRef.params.A4Freq);
        rePrepareNotes();
    }

    void updateTheme() {
        Theme::setTheme(processorRef.params.themeType);
        customLF->updateColors();
        smallLF.updateColors();
        mainViewport->updateColors();
        pitchMemorySettingsPanel->updateColors();
        sendLookAndFeelChange();
    }

    void leftClickTimeSnapButton() { timeSnapButton->triggerLeftClick(); }

    void leftClickKeySnapButton() { keySnapButton->triggerLeftClick(); }

    void leftClickEditRatiosMarksButton() { editRatiosMarksButton->triggerLeftClick(); }

    void leftClickPitchMemoryButton() { pitchMemoryButton->triggerLeftClick(); }

    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void fileDragEnter(const juce::StringArray &files, int x, int y) override;
    void fileDragExit(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;

    SmallLookAndFeel smallLF;

  private:
    AudioPluginAudioProcessor &processorRef;

    std::shared_ptr<DissonanceMeter> dissonanceMeter;

    std::shared_ptr<PitchMemory> pitchMemory;
    std::atomic<bool> pitchMemoryTerminate = false;
    std::unique_ptr<juce::ThreadPool> pitchMemoryThreadPool;

    std::unique_ptr<CustomLookAndFeel> customLF;

    std::unique_ptr<juce::Viewport> leftViewport, topViewport;
    std::unique_ptr<MainViewport> mainViewport;
    std::unique_ptr<LeftPanel> leftPanel;
    std::unique_ptr<TopPanel> topPanel;
    std::unique_ptr<MainPanel> mainPanel;

    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    std::unique_ptr<InstancesMenu> instancesMenu;
    std::unique_ptr<GenNewKeysMenu> genNewKeysMenu;
    std::unique_ptr<EditRatiosMarksMenu> editRatiosMarksMenu;
    std::unique_ptr<VocalToNotesMenu> vocalToNotesMenu;

    std::unique_ptr<MoreToolsMenu> moreToolsMenu;

    std::unique_ptr<VelocityPanel> velocityPanel;

    std::unique_ptr<juce::FileChooser> importFileChooser, exportFileChooser;

    std::unique_ptr<PopupMessage> popup;
    std::unique_ptr<DragAndDropPopup> dragAndDropPopup;
    std::unique_ptr<HelpViewport> helpViewport;
    std::unique_ptr<HelpPanel> helpPanel;
    std::unique_ptr<SettingsPanel> settingsPanel;
    std::unique_ptr<DissonancePanel> dissonancePanel;
    std::unique_ptr<PitchMemorySettingsPanel> pitchMemorySettingsPanel;

    std::unique_ptr<SVGButton> settingsButton, helpButton, timeSnapButton, keySnapButton,
        importButton, exportButton, camOnPlayHeadButton, turnOnAllZonesButton,
        turnOffAllZonesButton, dissonanceButton, pitchMemorySettingsButton, pitchMemoryButton,
        keysHarmonicityButton, ghostNotesKeysButton, ghostNotesTabButton, generateNewKeysButton,
        hideCentsButton, editRatiosMarksButton, moreToolsTabButton, notesFromGhostNotesButton,
        vocalToNotesButton;
    std::unique_ptr<juce::Label> numSubdivsLabel, numBeatsLabel, numBarsLabel, midiChannelLabel;
    std::unique_ptr<IntegerInput> numSubdivsInput, numBeatsInput, numBarsInput;

    const int timerMs = static_cast<int>(1000.0 / 60);
    const int ghostNotesUpdMs = 500;
    const int ghostNotesTimerTicks = ghostNotesUpdMs / timerMs;
    int ghostNotesTicker = 0;
    float playHeadTime = 0.0f;

    float octave_height_px = 300.0f;
    float bar_width_px = 400.0f;
    const int leftPanel_width_px = 150;
    const int topPanel_height_px = 50;
    const int slider_width_px = 8;
    const int bottom_x = 14;
    const int bottom_height_px = 30;
    const int buttons_gap_width_px = 14;
    const int bottom_gap_height_px = 12;
    const int popup_width_px = 300;
    const int popup_height_px = 50;
    const int dnd_popup_width_px = 500;
    const int dnd_popup_height_px = 250;
    const int top_height_px = bottom_height_px;
    const int top_x = (topPanel_height_px - top_height_px) / 2;
    const int top_y = top_x;

    // ============= vocal to notes =============
    bool wasVocalToNotes = false;
    int prevVocalNotesSize = -1;
    bool wasRecNote = false;
    float recVolume_dB = -60.0f;
    bool wasPlaying = false;
    // ==========================================

    const int velocity_width_px = 120;
    const int velocity_height_px = 40;
    const int velocity_bot_gap_px = 20;

    void parseMidiSclFiles(const juce::File &midiFile, const juce::File &sclFile = juce::File{});
    void importMidiSclFiles();
    void exportMidiSclFiles();
    void parseNotesFile(const juce::File &notesFile);
    void importNotesFile();
    void exportNotesFile();

    void processDroppedFiles(const juce::StringArray &files);

    float getTextWidth(const juce::String &text, const juce::Font &font) {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, text, 0, 0);
        return glyphs.getBoundingBox(0, -1, true).getWidth();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
} // namespace audio_plugin
