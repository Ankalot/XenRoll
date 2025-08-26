#pragma once

#include "BinaryData.h"
#include "DissonanceMeter.h"
#include "DissonancePanel.h"
#include "HelpPanel.h"
#include "IntegerInput.h"
#include "LeftPanel.h"
#include "MainPanel.h"
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

    void updateColors() {
        // Default colors for Tooltip
        setColour(juce::TooltipWindow::backgroundColourId, Theme::darkest);
        setColour(juce::TooltipWindow::outlineColourId, Theme::brightest);
        setColour(juce::TooltipWindow::textColourId, Theme::brightest);

        // Default colors for Label
        setColour(juce::Label::textColourId, Theme::brightest);
        setColour(juce::Label::backgroundColourId, Theme::darker);

        // Default colors for TextEditor
        setColour(juce::TextEditor::textColourId, Theme::brightest);
        setColour(juce::TextEditor::backgroundColourId, Theme::darkest);
        setColour(juce::TextEditor::outlineColourId, Theme::darkest);

        // Default colors for ComboBox
        setColour(juce::ComboBox::backgroundColourId, Theme::darkest);
        setColour(juce::ComboBox::textColourId, Theme::brightest);
        setColour(juce::ComboBox::outlineColourId, Theme::darkest);
        setColour(juce::ComboBox::buttonColourId, Theme::darkest);
        setColour(juce::ComboBox::arrowColourId, Theme::brighter);
        setColour(juce::ComboBox::focusedOutlineColourId, Theme::brighter);

        setColour(juce::PopupMenu::backgroundColourId, Theme::darkest);
        setColour(juce::PopupMenu::textColourId, Theme::brightest);
        setColour(juce::PopupMenu::headerTextColourId, Theme::brightest);
        setColour(juce::PopupMenu::highlightedTextColourId, Theme::brightest);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, Theme::dark);

        // Default colors for Slider
        setColour(juce::Slider::backgroundColourId, Theme::darkest);
        setColour(juce::Slider::thumbColourId, Theme::brighter);
        setColour(juce::Slider::trackColourId, Theme::darkest);
        setColour(juce::Slider::textBoxTextColourId, Theme::brightest);
        setColour(juce::Slider::textBoxBackgroundColourId, Theme::darkest);
        setColour(juce::Slider::textBoxOutlineColourId, Theme::darkest);

        // Default colors for TextButton
        setColour(juce::TextButton::buttonColourId, Theme::bright);
        setColour(juce::TextButton::textColourOffId, Theme::darkest);

        // Default colors for AlertWindow
        setColour(juce::AlertWindow::backgroundColourId, Theme::darkest);
        setColour(juce::AlertWindow::textColourId, Theme::brightest);
        setColour(juce::AlertWindow::outlineColourId, Theme::darkest);
    }

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

class MainViewport : public juce::Viewport {
  public:
    MainViewport(juce::Viewport *leftViewport, juce::Viewport *topViewport)
        : leftViewport(leftViewport), topViewport(topViewport) {updateColors();}

    void updateColors() {
        getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, Theme::bright);
        getHorizontalScrollBar().setColour(juce::ScrollBar::thumbColourId, Theme::bright);
    }

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

  private:
    juce::Viewport *leftViewport;
    juce::Viewport *topViewport;

    void visibleAreaChanged(const juce::Rectangle<int> &newVisibleArea) override {
        Viewport::visibleAreaChanged(newVisibleArea);
        leftViewport->setViewPosition(leftViewport->getViewPositionX(), newVisibleArea.getY());
        topViewport->setViewPosition(newVisibleArea.getX(), topViewport->getViewPositionY());
    }
};

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
  public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &);
    ~AudioPluginAudioProcessorEditor() override;

    void paint(juce::Graphics &) override;
    void resized() override;

    void repaintTopPanel() { topPanel->repaint(); }

    void changeOctaveHeightPx(int new_octave_height_px) {
        octave_height_px = new_octave_height_px;
        leftPanel->changeOctaveHeightPx(octave_height_px);
    }

    void changeBeatWidthPx(int new_bar_width_px) {
        bar_width_px = new_bar_width_px;
        topPanel->changeBeatWidthPx(bar_width_px);
    }

    void updateKeys(std::set<int> keys) { leftPanel->updateKeys(keys); }

    void showMessage(const juce::String &message) { popup->showMessage(message); }

    void zonesChanged() {
        mainPanel->remakeKeys();
        mainPanel->repaint();
        updatePitchMemory();
    }

    void hideMessage() { popup->timerCallback(); }

    const juce::String &getTextFromMessage() { return popup->getText(); }

    void updateNotes(const std::vector<Note> &new_notes) {
        processorRef.updateNotes(new_notes);
        updatePitchMemory();
    }

    void updatePitchMemory() {
        bool showKeysHarmonicity = processorRef.params.showKeysHarmonicity;
        if (processorRef.params.showPitchesMemoryTraces || showKeysHarmonicity) {
            // ====================================================================================
            //                         THIS THREAD POOL CAN CAUSE PROBLEMS?
            // ====================================================================================
            // 0. Stop finding PitchMemoryResults
            pitchMemoryTerminate.store(true);
            pitchMemoryThreadPool->removeAllJobs(true, 0);
            // 1. Get notes
            std::vector<Note> notes = getNotes();
            // 2. Remove notes that are not in active zones
            notes.erase(
                std::remove_if(notes.begin(), notes.end(),
                               [this](const Note &note) {
                                   return !this->processorRef.params.zones.isNoteInActiveZone(note);
                               }),
                notes.end());
            // 3. Add job where PitchMemoryResults will be found
            pitchMemoryThreadPool->addJob([this, notes, showKeysHarmonicity]() {
                pitchMemoryTerminate.store(false);
                this->pitchMemory->set_TV_add_influence(
                    this->processorRef.params.pitchMemoryTVaddInfluence);
                this->pitchMemory->set_TV_min_nonzero(
                    this->processorRef.params.pitchMemoryTVminNonzero);
                this->pitchMemory->set_TV_val_for_zero_HV(
                    this->processorRef.params.pitchMemoryTVvalForZeroHV);
                auto pitchMemoryResults =
                    this->pitchMemory->findPitchTraces(notes, this->pitchMemoryTerminate);
                if (pitchMemoryResults.has_value()) {
                    if (showKeysHarmonicity) {
                        auto keysHarmonicity = this->pitchMemory->findKeysHarmonicity(
                            pitchMemoryResults.value(), this->pitchMemoryTerminate);
                        if (keysHarmonicity.has_value()) {
                            juce::MessageManager::callAsync([this, keysHarmonicity]() {
                                this->leftPanel->updateKeysHarmonicity(keysHarmonicity.value());
                            });
                        }
                    }
                    juce::MessageManager::callAsync([this, pitchMemoryResults]() {
                        this->mainPanel->updatePitchMemoryResults(pitchMemoryResults.value());
                    });
                }
            });
        }
    }

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

    void timerCallback() {
        float newPlayHeadTime = processorRef.getPlayHeadTime();
        leftPanel.get()->setAllCurrPlayedNotesTotalCents(
            processorRef.getAllCurrPlayedNotesTotalCents());
        if (newPlayHeadTime != playHeadTime) {
            playHeadTime = newPlayHeadTime;
            mainPanel->setPlayHeadTime(playHeadTime);
            topPanel->setPlayHeadTime(playHeadTime);
            if (processorRef.params.isCamFixedOnPlayHead) {
                mainViewport->setCamOnTime(playHeadTime, bar_width_px);
            }
        }

        bool pitchOverflow = processorRef.thereIsPitchOverflow();
        if (pitchOverflow) {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon, "Pitches Overflow",
                juce::String(
                    "You have exceeded the limit on the number of unique pitches (128). ") +
                    "This number includes all notes from the piano roll and those that are "
                    "played " +
                    "manually (using mouse, keyboard or midi controller) with a unique pitch.\n\n" +
                    "FIX: remove some notes and/or don't manually play that many keys on which " +
                    "you don't have notes from the piano roll.",
                "OK");
        }
    }

    void bringBackKeyboardFocus() { mainPanel->grabKeyboardFocus(); }

    void A4freqChanged() {
        dissonanceMeter->setA4freq(processorRef.params.A4Freq);
        rePrepareNotes();
    }

    void updateTheme() {
        Theme::setTheme(processorRef.params.themeType);
        customLF->updateColors();
        mainViewport->updateColors();
        pitchMemorySettingsPanel->updateColors();
        // without this sliders' textboxes wouldn't update
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
        juce::LookAndFeel::setDefaultLookAndFeel(customLF.get());
        repaint();
    }

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

    std::unique_ptr<VelocityPanel> velocityPanel;

    std::unique_ptr<juce::FileChooser> importFileChooser, exportFileChooser;

    std::unique_ptr<PopupMessage> popup;
    std::unique_ptr<HelpViewport> helpViewport;
    std::unique_ptr<HelpPanel> helpPanel;
    std::unique_ptr<SettingsPanel> settingsPanel;
    std::unique_ptr<DissonancePanel> dissonancePanel;
    std::unique_ptr<PitchMemorySettingsPanel> pitchMemorySettingsPanel;

    std::unique_ptr<SVGButton> settingsButton, helpButton, timeSnapButton, keySnapButton,
        importButton, exportButton, camOnPlayHeadButton, turnOnAllZonesButton,
        turnOffAllZonesButton, dissonanceButton, pitchMemorySettingsButton, pitchMemoryButton,
        keysHarmonicityButton;
    std::unique_ptr<juce::Label> numSubdivsLabel, numBeatsLabel, numBarsLabel, midiChannelLabel;
    std::unique_ptr<IntegerInput> numSubdivsInput, numBeatsInput, numBarsInput;

    const int timerMs = int(1000.0 / 60);
    float playHeadTime = 0.0f;

    int octave_height_px = 300;
    int bar_width_px = 400;
    const int leftPanel_width_px = 150;
    const int topPanel_height_px = 50;
    const int slider_width_px = 8;
    const int bottom_x = 15;
    const int bottom_height_px = 32;
    const int bottom_gap_height_px = 15;
    const int popup_width_px = 300;
    const int popup_height_px = 50;
    const int top_height_px = bottom_height_px;
    const int top_x = (topPanel_height_px - top_height_px) / 2;
    const int top_y = top_x;

    const int velocity_width_px = 120;
    const int velocity_height_px = 40;
    const int velocity_bot_gap_px = 20;

    void importMidiSclFiles();
    void exportMidiSclFiles();
    void importNotesFile();
    void exportNotesFile();

    float getTextWidth(const juce::String &text, const juce::Font &font) {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, text, 0, 0);
        return glyphs.getBoundingBox(0, -1, true).getWidth();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
} // namespace audio_plugin
