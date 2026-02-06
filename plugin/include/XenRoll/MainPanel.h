#pragma once

#include "CircularStack.h"
#include "Note.h"
#include "PitchMemory.h"
#include "Theme.h"
#include <algorithm>
#include <deque>
#include <mutex>
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
    void updateRatiosMarks();
    void numBarsChanged();
    void setVelocitiesOfSelectedNotes(float vel);
    std::pair<int, int> findNearestKey(int cents, int octave);
    std::optional<int> findNearestKeyWithLimit(int key, int maxCentsChange, const std::set<int>& keys);

    void updatePitchMemoryResults(const PitchMemoryResults &newPitchMemoryResults);

  private:
    float playHeadTime;
    const int max_bar_width_px = 1000;
    int init_octave_height_px, init_bar_width_px;
    int octave_height_px, bar_width_px;
    bool isDragging = false;
    bool isResizing = false;
    bool isMoving = false;
    bool isSelecting = false;
    bool wasResizing = false;
    bool wasMoving = false;
    bool isDrawingRatioMark = false;
    juce::Point<int> lastDragPos;
    juce::Point<int> lastViewPos;
    juce::Point<int> selectStartPos;
    juce::Rectangle<int> selectionRect;
    juce::Point<int> ratioMarkStartPos;
    juce::Point<int> ratioMarkLastPos;
    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;

    // is needed when placing a note with params->playDraggedNotes
    std::map<int, int> placedNoteKeyCounter;
    std::mutex mptcMtx;

    juce::Font bendFont;

    PitchMemoryResults pitchMemoryResults = {{}, {}};

    const juce::String keysPlaySet = "zxcvbnm,./asdfghjkl;qwertyuiop";
    std::set<int> manuallyPlayedKeysTotalCents;
    std::set<int> wasKeyDown;

    // new should be added at the end (with push_back) (don't remember why lol)
    std::vector<Note> notes;
    std::set<int> keys;
    std::set<int> allAllKeys; // needed when params->autoCorrectRatiosMarks
    std::array<bool, 1200> keyIsGenNew;
    // ==================== Needed for generating new keys ====================
    void generateNewKeys();
    // === For Parameters::GenNewKeysTactics::Random ===
    std::array<bool, 1200> pickableKeys;
    // === For Parameters::GenNewKeysTactics::DiverseIntervals  ===
    std::array<bool, 1200> intervalsWere;
    std::array<int, 1200> intervalsDist; // metric for intervals
    // Possible new keys: 0, 10, 20, ..., 1180, 1190
    // This is calculated from intervalsDist. final weight = combination of
    //     intervalsDist and keysDist
    std::array<int, 120> possibleNewKeysWeights;
    // ========================================================================
    std::vector<Note> copiedNotes;
    CircularStack<std::vector<Note>> notesHistory{20};
    const int note_right_corner_width = 5;
    const int numDashLengths = 2;
    const float dashLengths[2] = {45, 15};
    int needToUnselectAllNotesExcept = -1;
    float lastDuration = 1.0f;
    float lastVelocity = 100.0f/127;
    float vertMoveSlowCoef = 0.2f;

    std::vector<Note> ghostNotes;

    std::pair<int,int> pointToOctaveCents(juce::Point<int> point);
    float adaptHor(float inputThickness);
    float adaptVert(float inputThickness);
    float adaptFont(float inputThickness);
    void updateLayout();
    juce::Path getNotePath(const Note &note);
    void deleteNote(int i);
    bool thereAreSelectedNotes();
    int getNumOfSelectedNotes();
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
    int dcents = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPanel)
};
} // namespace audio_plugin