#pragma once

#include "CircularStack.h"
#include "Note.h"
#include "PitchMemory.h"
#include "Theme.h"
#include <algorithm>
#include <deque>
#include <juce_audio_processors/juce_audio_processors.h>
#include <mutex>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

/**
 * @brief The main panel on which the notes are located. Something like a canvas.
 *
 */
class MainPanel : public juce::Component, public juce::KeyListener {
  public:
    MainPanel(float octave_height_px, float bar_width_px, AudioPluginAudioProcessorEditor *editor,
              Parameters *params);
    ~MainPanel() override;

    ///< NO REPAINT!
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
                          const juce::Font &font);
    void paint(juce::Graphics &g) override;

    void unselectAllNotes();

    void quantizeSelectedNotes();
    void randomizeSelectedNotesTiming();
    void randomizeSelectedNotesVelocity();
    void deleteAllRatiosMarks();

    void updateNotes(const std::vector<Note> &new_notes);
    void updateGhostNotes(const std::vector<Note> &new_ghostNotes);
    void createNotesFromGhostNotes();
    void remakeKeys();

    ///< Add recorded vocal notes to normal notes (when recording is over). NO REPAINT!
    void addVocalNotes(const std::vector<Note> &newVocalNotes);
    ///< Update recorded vocal notes (but don't mix them with normal ones yet). NO REPAINT!
    void updateVocalNotes(const std::vector<Note> &newVocalNotes);
    ///< Hide currently recording vocal notes. NO REPAINT!
    void hideRecNote();
    ///< Update currently recording vocal note. NO REPAINT!
    void updateRecNote(const Note &newRecNote);

    void clearPitchCurve() {
        pitchCurve.first.clear();
        pitchCurve.second.clear();
        repaint();
    };

    PitchCurve& getPitchCurveRef() { return pitchCurve; };

    void changeBarWidthPx(float new_bar_width_px) {
        bar_width_px = new_bar_width_px;
        updateLayout();
    }

    void changeOctaveHeightPx(float new_octave_height_px) {
        octave_height_px = new_octave_height_px;
        updateLayout();
    }

    /**
     * @brief Trying to attach ratio marks that lost their keys
     * @todo Change this later maybe, that's a lazy solution
     */
    void reattachRatiosMarks();
    void updateRatiosMarks();
    void numBarsChanged();

    /**
     * @brief Set velocity of selected notes
     * @param vel Velocity value (0-1)
     */
    void setVelocitiesOfSelectedNotes(float vel);

    /**
     * @brief Find nearest key to a given pitch
     * @param cents Cents within octave
     * @param octave Octave number
     * @return Pair of (octave, cents) of nearest key
     */
    std::pair<int, int> findNearestKey(int cents, int octave);

    /**
     * @brief Find nearest key within maximum cents change
     * @param key Cents of the key
     * @param maxCentsChange Maximum cents change allowed
     * @param keys Set of available keys
     * @return Optional total cents of nearest key, or nullopt if none found
     */
    std::optional<int> findNearestKeyWithLimit(int key, int maxCentsChange,
                                               const std::set<int> &keys);

    void updatePitchMemoryResults(const PitchMemoryResults &newPitchMemoryResults);

  private:
    float playHeadTime; ///< In bars
    const float max_bar_width_px = 1000.0f;
    float init_octave_height_px, init_bar_width_px;
    float octave_height_px, bar_width_px;
    bool isDragging = false;
    bool isResizing = false;
    bool isMoving = false;
    bool isSelecting = false;
    bool wasResizing = false;
    bool wasMoving = false;
    bool isDrawingRatioMark = false;
    bool isMovingRatioMark = false;
    bool prevDragPointIsActual = false;
    RatioMark *movingRatioMark;
    juce::Point<int> prevDragPoint;
    juce::Point<int> lastDragPos;
    juce::Point<int> lastViewPos;
    juce::Point<int> selectStartPos;
    juce::Rectangle<int> selectionRect;
    juce::Point<int> ratioMarkStartPos;
    juce::Point<int> ratioMarkLastPos;
    AudioPluginAudioProcessorEditor *editor;
    Parameters *params;

    ///< is needed when placing a note with params->playDraggedNotes
    std::map<int, int> placedNoteKeyCounter;
    std::mutex mptcMtx;

    juce::Font bendFont;

    PitchMemoryResults pitchMemoryResults = {{}, {}};

    const juce::String keysPlaySet = "zxcvbnm,./asdfghjkl;qwertyuiop";
    std::set<int> manuallyPlayedKeysTotalCents;
    std::map<juce::juce_wchar, int> wasKeyDown; ///< char and totalCents

    // new should be added at the end (with push_back) (don't remember why lol)
    std::vector<Note> notes;
    std::set<int> keys;
    ///< Keys from ALL notes and ghost notes (is needed when params->autoCorrectRatiosMarks)
    std::set<int> keysFromAllNotes;
    std::array<bool, 1200> keyIsGenNew;
    // ==================== Needed for generating new keys ====================
    void generateNewKeys();
    // === For Parameters::GenNewKeysTactics::Random ===
    std::array<bool, 1200> pickableKeys;
    // === For Parameters::GenNewKeysTactics::DiverseIntervals  ===
    std::array<bool, 1200> intervalsWere;
    std::array<int, 1200> intervalsDist; ///< metric for intervals
    /**
     * Possible new keys: 0, 10, 20, ..., 1180, 1190
     * This is calculated from intervalsDist. final weight = combination of intervalsDist and
     * keysDist
     */
    std::array<int, 120> possibleNewKeysWeights;
    // ========================================================================
    std::vector<Note> copiedNotes;
    CircularStack<std::vector<Note>> notesHistory{20};
    const int note_right_corner_width = 5;
    const int numDashLengths = 2;
    const float dashLengths[2] = {45, 15};
    const int ratioMarkHalfWidth = 5;
    const int ratioMarkMinHeight = 10; ///< is needed for deleting small ratio marks like 1/1
    int needToUnselectAllNotesExcept = -1;
    float lastDuration = 1.0f;
    float lastVelocity = 100.0f / 127;
    float vertMoveSlowCoef = 0.2f;

    // ============================= VOCAL TO MELODY =============================
    std::vector<Note> vocalNotes;
    Note recNote;
    bool showRecNote = false;
    PitchCurve pitchCurve;
    // ==========================================================================

    std::vector<Note> ghostNotes;

    std::pair<int, int> pointToOctaveCents(juce::Point<int> point);

    /**
     * @brief Adapt horizontal thickness based on zoom level
     * @param inputThickness Input thickness
     * @return Adapted thickness
     */
    float adaptHor(float inputThickness);

    /**
     * @brief Adapt vertical thickness based on zoom level
     * @param inputThickness Input thickness
     * @return Adapted thickness
     */
    float adaptVert(float inputThickness);

    /**
     * @brief Adapt font size based on zoom level
     * @param inputThickness Input font size
     * @return Adapted font size
     */
    float adaptFont(float inputThickness);
    void updateLayout();

    /**
     * @brief Get the path for drawing a note
     * @param note Note to get path for
     * @return Path representing the note shape
     */
    juce::Path getNotePath(const Note &note);

    /**
     * @brief Delete a note by index
     * @param i Index of note to delete
     */
    void deleteNote(int i);
    bool thereAreSelectedNotes();
    int getNumOfSelectedNotes();

    /**
     * @brief Get cents value from the last message text
     * @return Cents value, or -1 if invalid
     */
    int getCentsFromMessage();

    /**
     * @brief Snap time to nearest subdivision
     * @param time Input time in bars
     * @return Snapped time in bars
     */
    float timeToSnappedTime(float time);

    /**
     * @brief Get index of a key in the keys set
     * @param cents Cents value (0-1199)
     * @return Index in keys set, or -1 if not found
     */
    int getKeyIndex(int cents);

    /**
     * @brief Convert octave and cents to octave and nearest key cents
     * @param octave Octave number
     * @param cents Cents within octave
     * @return Tuple of (octave, cents of nearest key)
     */
    std::tuple<int, int> centsToKeysCents(int octave, int cents);
    bool doesPathIntersectRect(const juce::Path &somePath, const juce::Rectangle<float> &rect);
    void selectAllNotes();

    bool pointOnRatioMark(const RatioMark &ratioMark, const juce::Point<int> &point);
    bool lineIntersectsRatioMark(const RatioMark &ratioMark, const juce::Line<int> &line);

    int totalCentsToY(int totalCents);

    float dtime = 0.0f;
    int dcents = 0;

    static const juce::PathStrokeType outlineStroke;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPanel)
};
} // namespace audio_plugin