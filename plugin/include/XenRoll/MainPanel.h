#pragma once

#include "CircularStack.h"
#include "Note.h"
#include "PitchMemory.h"
#include "Theme.h"
#include <algorithm>
#include <deque>
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

class MainPanel : public juce::Component, public juce::KeyListener {
  public:
    MainPanel(int octave_height_px, int bar_width_px, AudioPluginAudioProcessorEditor *editor,
              Parameters *params);
    ~MainPanel() override;

    void setPlayHeadTime(float newPlayHeadTime);

    void mouseWheelMove(const juce::MouseEvent &event,
                        const juce::MouseWheelDetails &wheel) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseMove(const juce::MouseEvent &) override;
    bool keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) override;
    bool keyStateChanged(bool isKeyDown) override;

    void drawOutlinedText(juce::Graphics &g, const juce::String &text, juce::Rectangle<float> area,
                          const juce::Font &font, float outlineThickness = Theme::narrow);
    void paint(juce::Graphics &g) override;

    void unselectAllNotes();

    void updateNotes(const std::vector<Note> &new_notes);
    void updateGhostNotes(const std::vector<Note> &new_ghostNotes);
    void remakeKeys();
    void numBarsChanged();
    void setVelocitiesOfSelectedNotes(float vel);

    void updatePitchMemoryResults(const PitchMemoryResults &newPitchMemoryResults);

  private:
    float playHeadTime;
    const int max_bar_width_px = 1000;
    int octave_height_px, bar_width_px;
    bool isDragging = false;
    bool isResizing = false;
    bool isMoving = false;
    bool isSelecting = false;
    bool wasResizing = false;
    bool wasMoving = false;
    juce::Point<int> lastDragPos;
    juce::Point<int> lastViewPos;
    juce::Point<int> selectStartPos;
    juce::Rectangle<int> selectionRect;
    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;

    juce::Font bendFont;

    PitchMemoryResults pitchMemoryResults = {{}, {}};

    const juce::String keysPlaySet = "zxcvbnm,./asdfghjkl;qwertyuiop";
    std::set<int> manuallyPlayedKeysTotalCents;
    std::set<int> wasKeyDown;

    // new should be added at the end (with push_back) (don't remember why lol)
    std::vector<Note> notes;
    std::set<int> keys;
    std::vector<Note> copiedNotes;
    CircularStack<std::vector<Note>> notesHistory{20};
    const int note_right_corner_width = 5;
    int needToUnselectAllNotesExcept = -1;
    float lastDuration = 1.0f;

    std::vector<Note> ghostNotes;

    void updateLayout();
    juce::Path getNotePath(const Note &note);
    void deleteNote(int i);
    bool thereAreSelectedNotes();
    // if invalid message returns -1
    int getCentsFromMessage();
    float timeToSnappedTime(float time);
    int getKeyIndex(int cents);
    std::tuple<int, int> centsToKeysCents(int octave, int cents);
    bool doesPathIntersectRect(const juce::Path &parallelogram,
                               const juce::Rectangle<float> &rect);
    void selectAllNotes();

    int totalCentsToY(int totalCents);

    float dtime = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPanel)
};
} // namespace audio_plugin