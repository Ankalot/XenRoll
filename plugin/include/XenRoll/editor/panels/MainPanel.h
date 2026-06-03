#pragma once

#include "XenRoll/data/Note.h"
#include "XenRoll/data/Theme.h"
#include "XenRoll/editor/models/PitchMemory.h"
#include "XenRoll/editor/panels/NotePathManager.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <mutex>

namespace audio_plugin {
class AudioPluginAudioProcessorEditor;

/**
 * @brief The main panel on which the notes are located. Something like a canvas.
 *
 */
class MainPanel : public juce::Component, public juce::KeyListener {
  public:
    MainPanel(AudioPluginAudioProcessorEditor &editor, Parameters &params);
    ~MainPanel() override;

    ///< CALL RIGHT AFTER CREATING MainViewport AND ASSIGNING MainPanel TO IT
    void initViewport();

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
    void modifierKeysChanged(const juce::ModifierKeys &modifiers) override;

    ///< For use in mouseMove and in mouseUp
    void updateMouseCursor(const juce::MouseEvent &event);

    void drawDoubleArrow(juce::Graphics &g, juce::Point<float> p1, juce::Point<float> p2,
                         float arrowWidth);
    void drawOutlinedText(juce::Graphics &g, const juce::String &text, juce::Rectangle<float> area,
                          const juce::Font &font);
    void paint(juce::Graphics &g) override;

    ///< NO REPAINT!
    void unselectAllNotes();

    void quantizeSelectedNotes();
    void randomizeSelectedNotesTiming();
    void randomizeSelectedNotesVelocity();
    void deleteAllRatiosMarks();
    void mirrorSelNotesHorizontally();
    void mirrorSelNotesVertically();

    // is called from PluginEditor when importing
    void updateNotes(const std::vector<Note> &new_notes);
    void updateGhostNotes(const std::vector<Note> &new_ghostNotes);
    void createNotesFromGhostNotes();
    /**
     * If remaking keys was caused just by pitch transposition, set dcents!
     * Contains reattachRatiosMarks method! So use it before saveState()
     */
    void remakeKeys(int dcents = 0);

    /**
     * @brief Add recorded notes to normal notes (when recordnig is over). This
     * method is for both vocal notes and manually played notes.
     * @note Automatically unselects all other notes. NO REPAINT!
     * @param newVocalNotes Recorded notes, must be selected.
     */
    void addRecordedNotes(const std::vector<Note> &recordedNotes);

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

    PitchCurve &getPitchCurveRef() { return pitchCurve; };

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
     * @param dcents If there was just pitch transposition, set dcents!
     * @note Should be called after correctRatioMarksBasedOnSelNotes()
     */
    void reattachRatiosMarks(int dcents = 0);
    void updateRatiosMarks();
    ///< For calls from PluginEditor. manualChange means not due to import.
    void numBarsChanged(bool manualChange = false);

    /**
     * @brief Set velocity of selected notes
     * @param vel Velocity value (0-1)
     */
    void setVelocitiesOfSelectedNotes(float vel);

    void updatePitchMemoryResults(const PitchMemoryResults &newPitchMemoryResults);

    const std::vector<Note> &getNotes() { return notes; }

    // Don't forget: call it after changes with ratio marks (corrections/reattachments, etc)
    void saveState() {
        params.stateHistory.push(State(params.get_num_bars(), notes, params.ratiosMarks));
    }

    void invalidateNotePathsCache() { notePathManager->invalidateCache(); }

  private:
    float playHeadTime; ///< In bars
    float init_octave_height_px, init_bar_width_px;
    float octave_height_px, bar_width_px;
    float auditionTime; ///< In bars
    bool isAuditioning = false;
    bool isPanning = false;
    bool isResizing = false;
    bool isMoving = false;
    bool isSelecting = false;
    bool isTimeStretching = false;
    bool wasResizing = false;
    bool wasMoving = false;
    bool wasTimeStretching = false;
    bool isDrawingRatioMark = false;
    bool isMovingRatioMark = false;
    bool wasMovingRatioMark = false;
    bool isDeletingRatioMarks = false;
    ///< for bending not up to keys! no "bend key snap"
    bool wasBending = false;
    ///< for changing velocity using mouse scroll
    bool wasVelocityChanging = false;
    ///< for small timestep change from keyboard only (left/right arrows & no time snap)
    bool wasTimeChanging = false;
    bool wasPitchChanging = false; ///< for ±1¢ from keyboard only (up/down arrows & no key snap)
    RatioMark *movingRatioMark;
    // Pos is relative to viewport
    juce::Point<int> startDragPos;
    juce::Point<int> lastDragPos;
    // Point is relative to panel
    juce::Point<int> startDragPoint;
    juce::Point<int> lastDragPoint;
    juce::Point<int> selectStartPoint;
    juce::Point<int> selectLastPoint;
    juce::Point<int> ratioMarkStartPoint;
    juce::Point<int> ratioMarkLastPoint;

    ///< Initial state of selected notes for moving, resizing and time-stretching: {time, duration}
    std::vector<std::pair<float, float>> initialStateForDrag;
    ///< Initial state of selected notes for moving: {totalCents}
    std::vector<int> initialTotalCentsForDrag;
    ///< Initial times of ratio marks that move due to time-stretch, or due to moving RatioMark
    std::vector<float> initialRatioMarkTimesForDrag;

    /**
     * x-position in bars when started dragging (for moving, resizing, time-stretching,
     * moving RatioMark)
     */
    float dragPivotTime = 0.0f;
    ///< y-position in octaves when started dragging (for moving)
    float dragPivotPitch = 0;
    /**
     * Index of note that was clicked at the start of resizing or time-stretching
     * (so can find and set lastDuration)
     */
    int resizeClickNoteInd = 0;
    ///< Initial border of selected notes when started time-stretching (in bars)
    float timeStretchSelectionLeft = 0.0f, timeStretchSelectionRight = 0.0f;

    float prevScale = 1.0f; ///< Previous scale when time-stretching
    float prevDtime = 0.0f; ///< Previous dtime when resizing or moving notes
    int prevDcents = 0;     ///< Previous dcents when moving notes
    float prevTime = -1.0f; ///< Previous time of ratio mark when moving it

    std::unique_ptr<NotePathManager> notePathManager;
    float noteRoundCoef; // for paint()

    AudioPluginAudioProcessorEditor &editor;
    Parameters &params;
    juce::Viewport *viewport;

    ///< is needed when placing a note with params.playDraggedNotes
    std::map<int, int> placedNoteKeyCounter;
    std::mutex mptcMtx;

    juce::Font bendFont;
    std::unordered_map<size_t, juce::Path> outlinedTextPathCache;
    static constexpr int OUTLINED_TEXT_CACHE_MAX_SIZE = 150;

    PitchMemoryResults pitchMemoryResults = {{}, {}};

    const juce::String keysPlaySet = "zxcvbnm,./asdfghjkl;qwertyuiop";
    std::map<int, float> keyboardManuallyPlayedKeys, dragManuallyPlayedKeys;
    std::map<juce::juce_wchar, int> wasKeyDown; ///< char and totalCents

    // New should be added at the end (with push_back).
    //     This is needed to maintain consistency over time. If several notes occupy the
    //     same place under the cursor, the action will be applied to the later added note.
    std::vector<Note> notes;
    std::set<int> keys;
    ///< Keys from ALL notes and ghost notes (is needed when params.autoCorrectRatiosMarks)
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
    const int note_right_corner_width = 5;
    const int numDashLengths = 2;
    const float dashLengths[2] = {45, 15};
    const int ratioMarkHalfWidth = 5;
    const int ratioMarkMinHeight = 10; ///< is needed for deleting small ratio marks like 1/1

    ///< This threshold will determine either user wants "drag action" or "click action"
    const int clickMoveThrPx = 2;
    int clickVelPanelNoteInd = -1;
    int clickDelNoteInd = -1;
    int clickUnselAllNotesExcept = -1;
    bool clickUnselAllNotesExcept_Ctrl = false;
    int needToUnselectThisNote = -1;
    bool needToUnselectThisNote_Ctrl = false;
    float vertMoveSlowCoef = 0.2f;

    ///< is changed inside showOrUpdateVelocityPanel() and hideVelocityPanel() only!
    bool isShowingVelocityPanel = false;
    void showOrUpdateVelocityPanel();
    void hideVelocityPanel();

    // ============================= VOCAL TO MELODY =============================
    std::vector<Note> vocalNotes;
    Note recNote;
    bool showRecNote = false;
    PitchCurve pitchCurve;
    // ==========================================================================

    std::vector<Note> ghostNotes;

    int getMaxCentsUpForSelNotes() const;   ///< returns 0+ cents
    int getMaxCentsDownForSelNotes() const; ///< returns 0- cents; max by abs

    ///< Find new {octave, cents} after raising by a key; keys MUST BE NOT EMPTY!
    std::pair<int, int> keyUp(int octave, int cents) const;
    ///< Find new {ocateve, cents} after lowering by a key; keys MUST BE NOTE EMPTY!
    std::pair<int, int> keyDown(int octave, int cents) const;

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
    /**
     * Is called when width or height is changed due to: num of bars changed / bar width
     * changed / octave height changed
     */
    void updateLayout();

    float getNotesHeight(); ///< in pixels

    ///< For fast check (for example if we need to draw note in paint())
    juce::Rectangle<float> getNoteBounds(const Note &note);

    // It is possible to return reference to canonical path (realtive to {0, 0}) and translation,
    //    and only then apply translation in juce::Graphics and draw path, but for some reason that
    //    doesn't speed up drawing, GPU doesn't cache same paths...
    /**
     * @brief Get the path for drawing a note
     * @param note Note to get path for
     * @return Path representing the note shape
     */
    juce::Path getNotePath(const Note &note);

    void drawNote(juce::Graphics &g, const Note &note, const juce::Rectangle<float> noteBounds,
                  const juce::PathStrokeType &strokeType, juce::Colour fillColour,
                  bool drawOutline = false, juce::Colour outlineColour = juce::Colours::black);

    /**
     * @brief Delete a note by index
     * @param i Index of note to delete
     */
    void deleteNote(int i);
    bool thereAreSelectedNotes();

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

    bool doesPathIntersectRect(const juce::Path &somePath, const juce::Rectangle<float> &rect);
    void selectAllNotes();

    bool pointOnNote(const Note &note, const juce::Point<float> &point);
    bool pointOnRatioMark(const RatioMark &ratioMark, const juce::Point<int> &point);
    bool lineIntersectsRatioMark(const RatioMark &ratioMark, const juce::Line<int> &line);

    /**
     * Set dtime if selected notes were moved horizontally!
     * Selected only notes - because only they have changed.
     * Better to call before reattachRatiosMarks()
     */
    void correctRatioMarksBasedOnSelNotes(float dtime = 0.0f);

    float totalCentsToY(int totalCents);

    void restoreState();

    bool wasDebugOverlayVisible = false;
    double minFps = std::numeric_limits<double>::max();
    juce::String pluginNameAndVersion;

    static const juce::PathStrokeType outlineStroke;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPanel)
};
} // namespace audio_plugin