#include "XenRoll/PluginEditor.h" // must be first!
#include "XenRoll/MainPanel.h"    // must be second!
#include <random>

namespace audio_plugin {
MainPanel::MainPanel(int octave_height_px, int bar_width_px,
                     AudioPluginAudioProcessorEditor *editor, Parameters *params)
    : octave_height_px(octave_height_px), bar_width_px(bar_width_px), editor(editor),
      params(params), init_octave_height_px(octave_height_px), init_bar_width_px(bar_width_px) {
    this->setSize(params->get_num_bars() * bar_width_px, params->num_octaves * octave_height_px);
    setInterceptsMouseClicks(true, true);

    addKeyListener(this);
    setWantsKeyboardFocus(true);

    notes = editor->getNotes();
    unselectAllNotes();
    remakeKeys();

    bendFont = juce::Font(getLookAndFeel().getTypefaceForFont(NULL)).withPointHeight(Theme::small_);
}

const juce::PathStrokeType MainPanel::outlineStroke(Theme::wide, juce::PathStrokeType::mitered);

MainPanel::~MainPanel() { removeKeyListener(this); }

int MainPanel::totalCentsToY(int totalCents) {
    return juce::roundToInt(octave_height_px * (params->num_octaves - totalCents / 1200.0));
}

void MainPanel::updatePitchMemoryResults(const PitchMemoryResults &newPitchMemoryResults) {
    pitchMemoryResults = newPitchMemoryResults;
    repaint();
}

void MainPanel::drawOutlinedText(juce::Graphics &g, const juce::String &text,
                                 juce::Rectangle<float> area, const juce::Font &font) {
    // Precompute once
    const float fontHeight = font.getHeight();
    const float areaCentreX = area.getCentreX();
    const float areaCentreY = area.getCentreY();

    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, text, 0.0f, fontHeight);

    juce::Path textPath;
    glyphs.createPath(textPath);

    // Get bounds and compute translation in one step
    const auto bounds = textPath.getBounds();
    textPath.applyTransform(juce::AffineTransform::translation(
        areaCentreX - (bounds.getX() + bounds.getWidth() * 0.5f),
        areaCentreY - (bounds.getY() + bounds.getHeight() * 0.5f)));

    g.setColour(Theme::darkest);
    g.strokePath(textPath, outlineStroke);

    g.setColour(Theme::brightest);
    g.fillPath(textPath);
}

float MainPanel::adaptHor(float inputThickness) {
    return inputThickness * std::min(1.0f, octave_height_px * 1.0f / init_octave_height_px);
}

float MainPanel::adaptVert(float inputThickness) {
    return inputThickness * std::min(1.0f, bar_width_px * 1.0f / init_bar_width_px);
}

float MainPanel::adaptFont(float inputThickness) {
    return inputThickness *
           std::min(std::min(1.0f, (octave_height_px + 1.0f * init_octave_height_px) * (0.5f) /
                                       init_octave_height_px),
                    std::min(1.0f, (bar_width_px + 1.0f * init_bar_width_px) * (0.5f) /
                                       init_bar_width_px));
}

void MainPanel::paint(juce::Graphics &g) {
    // We paint only visible area

    auto *viewport = findParentComponentOfClass<juce::Viewport>();
    if (viewport == nullptr)
        return;
    juce::Rectangle<int> clip = viewport->getViewArea();
    int clipWidth = clip.getWidth();
    int clipHeight = clip.getHeight();
    int clipX = clip.getX();
    int clipY = clip.getY();
    g.setColour(Theme::dark);
    g.fillRect(clip.toFloat());

    g.setColour(Theme::darkest);
    // octaves
    int octave_i_start = clipY / octave_height_px;
    int octave_i_end = std::min((clipY + clipHeight) / octave_height_px + 1, params->num_octaves);
    for (int i = octave_i_start; i <= octave_i_end; ++i) {
        float yPos = float(i * octave_height_px);
        g.drawLine(float(clipX), yPos, float(clipX + clipWidth), yPos, adaptHor(Theme::wide));
    }
    // bars
    int bar_i_start = clipX / bar_width_px;
    int bar_i_end = std::min((clipX + clipWidth) / bar_width_px + 1, params->get_num_bars());
    for (int i = bar_i_start; i <= bar_i_end; ++i) {
        float xPos = float(i * bar_width_px);
        g.drawLine(xPos, 0.0f, xPos, float(octave_height_px * params->num_octaves),
                   adaptVert(Theme::wide));
    }
    // beats and subdivisions
    for (int i = bar_i_start; i < bar_i_end; ++i) {
        for (int j = 0; j < params->num_beats; ++j) {
            float xPos = (i + float(j) / params->num_beats) * bar_width_px;
            g.drawLine(xPos, 0.0f, xPos, float(octave_height_px * params->num_octaves),
                       adaptVert(Theme::narrow));
            for (int k = 1; k < params->num_subdivs; ++k) {
                float xPosSub =
                    xPos + float(k) / (params->num_subdivs * params->num_beats) * bar_width_px;
                g.drawLine(xPosSub, 0.0f, xPosSub, float(octave_height_px * params->num_octaves),
                           adaptVert(Theme::narrower));
            }
        }
    }
    // keys
    for (const int &key : keys) {
        for (int j = octave_i_start; j < octave_i_end; ++j) {
            float yPos = (j + 1.0f - float(key) / 1200) * octave_height_px;
            auto line =
                juce::Line<float>(0.0f, yPos, float(params->get_num_bars() * bar_width_px), yPos);
            if (params->generateNewKeys && keyIsGenNew[key]) {
                g.drawDashedLine(line, dashLengths, numDashLengths, adaptHor(Theme::narrow));
            } else {
                g.drawLine(line, adaptHor(Theme::narrow));
            }
        }
    }

    // PlayHead line
    if ((playHeadTime * bar_width_px >= clipX) &&
        (playHeadTime * bar_width_px <= clipX + clipWidth)) {
        // g.setColour(Theme::brighter);
        g.setColour(Theme::activated);
        g.drawLine(float(playHeadTime * bar_width_px), float(clipY),
                   float(playHeadTime * bar_width_px), float(clipY + clipHeight), Theme::narrower);
    }

    auto clipFloat = clip.toFloat();
    juce::PathStrokeType strokeType(Theme::narrower);
    strokeType.setJointStyle(juce::PathStrokeType::mitered);

    // ghost notes
    for (const Note &note : ghostNotes) {
        juce::Path notePath = getNotePath(note);
        // is needed only when params->showPitchesMemoryTraces
        if (notePath.getBounds().intersects(clipFloat)) {
            g.setColour(Theme::darkest.interpolatedWith(Theme::darker, 0.5));
            g.fillPath(notePath);
        }
    }

    // notes
    int j = 0;
    for (const Note &note : notes) {
        juce::Path notePath = getNotePath(note);
        // is needed only when params->showPitchesMemoryTraces
        bool inActiveZone = params->zones.isNoteInActiveZone(note);
        if (notePath.getBounds().intersects(clipFloat)) {
            if (params->showPitchesMemoryTraces && inActiveZone) {
                if (j < pitchMemoryResults.second.size()) {
                    float noteHarm = pitchMemoryResults.second[j];
                    if (noteHarm > 0) {
                        g.setColour(
                            Theme::midHarmony.interpolatedWith(Theme::maxHarmony, noteHarm));
                    } else {
                        g.setColour(
                            Theme::midHarmony.interpolatedWith(Theme::minHarmony, -noteHarm));
                    }
                }
            } else {
                g.setColour(
                    Theme::dark.interpolatedWith(Theme::brighter.brighter(1.0f), note.velocity));
            }
            g.fillPath(notePath);
            if (note.isSelected) {
                g.setColour(Theme::brightest);
            } else {
                g.setColour(Theme::darkest);
            }
            g.strokePath(notePath, strokeType);
        }

        if (inActiveZone) {
            j++;
        }
    }
    // note bend text
    if (!params->hideCents) {
        for (const Note &note : notes) {
            juce::Path notePath = getNotePath(note);
            if (notePath.getBounds().intersects(clipFloat)) {
                if ((note.bend != 0) && note.isSelected) {
                    juce::String bendText = juce::String::formatted(
                        "%s%d", (note.bend > 0 ? "+" : "-"), abs(note.bend));
                    drawOutlinedText(g, bendText, notePath.getBounds(), bendFont);
                }
            }
        }
    }

    // Pitch traces
    if (params->showPitchesMemoryTraces && !params->pitchMemoryShowOnlyHarmonicity) {
        const auto &pitchTraces = pitchMemoryResults.first;

        const int numPitches = pitchTraces.first.size();
        std::vector<float> timeStamps;
        const int numTimeStamps = pitchTraces.second.size();
        timeStamps.reserve(numTimeStamps);

        std::transform(pitchTraces.second.begin(), pitchTraces.second.end(),
                       std::back_inserter(timeStamps), [](const auto &pair) { return pair.first; });

        int i = 0;
        for (const auto &pts : pitchTraces.second) {
            const float timeStart = timeStamps[i];
            float timeEnd = params->get_num_bars();
            if (i + 1 != numTimeStamps) {
                timeEnd = timeStamps[i + 1];
            }
            const int posXstart = timeStart * bar_width_px;
            const int posXend = timeEnd * bar_width_px;
            if ((posXstart <= clipX + clipWidth) && (posXend >= clipX)) {
                for (int j = 0; j < numPitches; ++j) {
                    const float traceValue = pts.second[j];
                    const int posY = totalCentsToY(pitchTraces.first[j]);
                    if ((posY >= clipY) && (posY <= clipY + clipHeight)) {
                        const juce::uint8 brightness = juce::roundToInt(255 * traceValue);
                        g.setColour(juce::Colour::fromRGB(brightness, brightness, brightness));
                        g.drawLine(posXstart, posY, posXend, posY, adaptHor(Theme::wider));
                    }
                }
            }
            i++;
        }
    }

    // selecting rectangle
    if (isSelecting) {
        g.setColour(Theme::darkest.withAlpha(0.5f));
        g.fillRect(selectionRect);

        g.setColour(Theme::brightest);
        g.drawRect(selectionRect, int(Theme::narrow));
    }

    // ratios marks
    g.setColour(Theme::activated);
    const auto fontSizeRatio = adaptFont(Theme::big);
    const auto fontSizeError = adaptFont(Theme::small_);
    for (const auto ratioMark : params->ratiosMarks) {
        float wdth = adaptVert(Theme::wide);
        float ratioMarkXPos = ratioMark.time * bar_width_px;
        if ((ratioMarkXPos >= clipX) && (ratioMarkXPos <= clipX + clipWidth)) {

            float lowerKeyY =
                (params->num_octaves - float(ratioMark.getLowerKeyTotalCents()) / 1200) *
                octave_height_px;
            float higherKeyY =
                (params->num_octaves - float(ratioMark.getHigherKeyTotalCents()) / 1200) *
                octave_height_px;

            // double arrow
            const auto point1 = juce::Point<float>(ratioMarkXPos, lowerKeyY);
            const float midY = (lowerKeyY + higherKeyY) / 2.0f;
            const auto point2 = juce::Point<float>(ratioMarkXPos, (lowerKeyY + higherKeyY) / 2.0f);
            const auto point3 = juce::Point<float>(ratioMarkXPos, higherKeyY);
            g.drawArrow(juce::Line<float>(point2, point1), wdth, wdth * 3, wdth * 3);
            g.drawArrow(juce::Line<float>(point2, point3), wdth, wdth * 3, wdth * 3);

            // ratio
            int num, den;
            std::tie(num, den) = ratioMark.getRatio();
            juce::String ratioMarkText = juce::String(num) + "/" + juce::String(den);
            g.setFont(fontSizeRatio);
            g.drawText(ratioMarkText,
                       juce::Rectangle<float>(ratioMarkXPos + adaptVert(5), midY - fontSizeRatio,
                                              100, fontSizeRatio),
                       juce::Justification::centredLeft);

            // error in cents
            int err = ratioMark.getError();
            if (err != 0) {
                juce::String errorMarkText = "";
                if (err > 0) {
                    errorMarkText += "+";
                }
                errorMarkText += juce::String(err) + "¢";
                g.setFont(fontSizeError);
                g.drawText(errorMarkText,
                           juce::Rectangle<float>(ratioMarkXPos + adaptVert(5), midY + adaptHor(3),
                                                  100, fontSizeError),
                           juce::Justification::centredLeft);
            }
        }
    }

    // ratio mark preline
    if (isDrawingRatioMark) {
        g.setColour(Theme::activated);
        float wdth = adaptVert(Theme::wide);
        const auto point1 = ratioMarkStartPos.toFloat();
        const auto point2 = juce::Point<float>(
            ratioMarkStartPos.getX(), (ratioMarkStartPos.getY() + ratioMarkLastPos.getY()) / 2.0f);
        const auto point3 = juce::Point<float>(ratioMarkStartPos.getX(), ratioMarkLastPos.getY());
        g.drawArrow(juce::Line<float>(point2, point1), wdth, wdth * 3, wdth * 3);
        g.drawArrow(juce::Line<float>(point2, point3), wdth, wdth * 3, wdth * 3);
    }
}

void MainPanel::unselectAllNotes() {
    for (int i = 0; i < notes.size(); ++i) {
        notes[i].isSelected = false;
    }
    repaint();
}

void MainPanel::quantizeSelectedNotes() {
    notesHistory.push(notes);
    float dt = 1.0f / (params->num_beats * params->num_subdivs);

    for (auto &note : notes) {
        if (note.isSelected) {
            float newTime = roundf(note.time / dt) * dt;
            float newDuration = roundf(note.duration / dt) * dt;
            if (newDuration < dt) {
                newDuration = dt;
            }
            note.time = newTime;
            note.duration = newDuration;
        }
    }

    editor->updateNotes(notes);
    remakeKeys();
    repaint();
}

void MainPanel::randomizeSelectedNotesTiming() {
    notesHistory.push(notes);
    float dt = 1.0f / (params->num_beats * params->num_subdivs);
    const float maxJitter = dt * 0.15f;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, maxJitter / 3.0f);

    for (auto &note : notes) {
        if (note.isSelected) {
            float jitter = juce::jlimit(-maxJitter, maxJitter, dist(gen));
            note.time += jitter;
            if (note.time < 0) {
                note.time = 0;
            }
        }
    }

    editor->updateNotes(notes);
    remakeKeys();
    repaint();
}

void MainPanel::randomizeSelectedNotesVelocity() {
    notesHistory.push(notes);
    const float maxJitter = 0.12f;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, maxJitter / 3.0f);

    for (auto &note : notes) {
        if (note.isSelected) {
            float jitter = juce::jlimit(-maxJitter, maxJitter, dist(gen));
            note.velocity = juce::jlimit(0.0f, 1.0f, note.velocity + jitter);
        }
    }

    editor->updateNotes(notes);
    repaint();
}

void MainPanel::numBarsChanged() {
    size_t notesNum = notes.size();
    for (int i = 0; i < notesNum; ++i) {
        if (notes[i].time + notes[i].duration > params->get_num_bars()) {
            deleteNote(i);
            i--;
            notesNum--;
        }
    }
    std::erase_if(params->ratiosMarks,
                  [&](const auto &ratioMark) { return (ratioMark.time > params->get_num_bars()); });
    int min_bar_width_px =
        static_cast<int>(round(getParentComponent()->getWidth() / params->get_num_bars())) + 1;
    if (bar_width_px < min_bar_width_px) {
        bar_width_px = min_bar_width_px;
        editor->changeBeatWidthPx(bar_width_px);
    }
    remakeKeys();
    updateLayout();
}

void MainPanel::mouseWheelMove(const juce::MouseEvent &event,
                               const juce::MouseWheelDetails &wheel) {
    if (event.mods.isAltDown()) {
        // notesHistory.push(notes);
        int multiplier = 1;
        if (event.mods.isShiftDown()) {
            multiplier = 10;
        }
        for (Note &note : notes) {
            if (note.isSelected) {
                note.bend += multiplier * (wheel.deltaY > 0 ? 1 : -1);
            }
        }
        editor->updateNotes(notes);
        repaint();
        return;
    }

    auto *viewport = findParentComponentOfClass<juce::Viewport>();
    if (viewport == nullptr)
        return;

    const int oldWidth = getWidth();
    const int oldHeight = getHeight();
    const auto viewPos = viewport->getViewPosition();
    const int viewWidth = viewport->getViewWidth();
    const int viewHeight = viewport->getViewHeight();

    const float centerX = (viewPos.x + viewWidth / 2.0f) / oldWidth;
    const float centerY = (viewPos.y + viewHeight / 2.0f) / oldHeight;

    float stretchFactor = 1.0f + wheel.deltaY * 0.5f;
    if (event.mods.isCtrlDown()) {
        octave_height_px = static_cast<int>(round(octave_height_px * stretchFactor));
        int min_octave_height_px =
            static_cast<int>(round(viewport->getHeight() / params->num_octaves));
        octave_height_px =
            juce::jlimit(min_octave_height_px, 15 * min_octave_height_px, octave_height_px);
        editor->changeOctaveHeightPx(octave_height_px);
    } else {
        bar_width_px = static_cast<int>(round(bar_width_px * stretchFactor));
        int min_bar_width_px =
            static_cast<int>(round(viewport->getWidth() / params->get_num_bars()));
        bar_width_px = juce::jlimit(min_bar_width_px, max_bar_width_px, bar_width_px);
        editor->changeBeatWidthPx(bar_width_px);
    }

    updateLayout();

    const int newWidth = getWidth();
    const int newHeight = getHeight();
    const int targetX = juce::roundToInt(centerX * newWidth - viewWidth / 2.0f);
    const int targetY = juce::roundToInt(centerY * newHeight - viewHeight / 2.0f);

    viewport->setViewPosition(juce::jlimit(0, juce::jmax(0, newWidth - viewWidth), targetX),
                              juce::jlimit(0, juce::jmax(0, newHeight - viewHeight), targetY));
}

std::pair<int, int> MainPanel::pointToOctaveCents(juce::Point<int> point) {
    int octave = params->num_octaves - 1 - point.getY() / octave_height_px;
    int cents = static_cast<int>(round(
                    (1.0f - (point.getY() % octave_height_px) * 1.0f / octave_height_px) * 1200)) %
                1200;
    if (cents == 0) {
        octave += 1;
        if (octave == params->num_octaves) {
            octave--;
        }
    }
    return std::make_pair(octave, cents);
}

bool MainPanel::pointOnRatioMark(const RatioMark &ratioMark, const juce::Point<int> &point) {
    float ratioMarkXPos = ratioMark.time * bar_width_px;
    if ((ratioMarkXPos - ratioMarkHalfWidth < point.getX()) &&
        (point.getX() < ratioMarkXPos + ratioMarkHalfWidth)) {
        float ratioMarkHighYPos =
            (params->num_octaves - float(ratioMark.getHigherKeyTotalCents()) / 1200) *
            octave_height_px;
        float ratioMarkLowYPos =
            (params->num_octaves - float(ratioMark.getLowerKeyTotalCents()) / 1200) *
            octave_height_px;
        float ratioMarkHeight = ratioMarkLowYPos - ratioMarkHighYPos;
        if (ratioMarkHeight < ratioMarkMinHeight) {
            ratioMarkLowYPos += (ratioMarkMinHeight - ratioMarkHeight) / 2.0f;
            ratioMarkHighYPos -= (ratioMarkMinHeight - ratioMarkHeight) / 2.0f;
        }
        return (ratioMarkHighYPos < point.getY()) && (point.getY() < ratioMarkLowYPos);
    }
    return false;
}

void MainPanel::mouseDown(const juce::MouseEvent &event) {
    grabKeyboardFocus();
    juce::Point<int> point = event.getPosition();

    if (event.mods.isMiddleButtonDown()) {
        if (!params->editRatiosMarks) {
            for (int i = int(notes.size()) - 1; i >= 0; --i) {
                juce::Path notePath = getNotePath(notes[i]);
                if (notePath.contains(point.toFloat())) {
                    notes[i].isSelected = true;
                    editor->showVelocityPanel(notes[i].velocity);
                    repaint();
                    return;
                }
            }
        }

        isDragging = true;
        // Get position in parent (viewport) coordinates
        lastDragPos = getParentComponent()->getLocalPoint(this, event.getPosition());
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }

    if (event.mods.isLeftButtonDown()) {
        if (params->editRatiosMarks) {
            for (auto &ratioMark : params->ratiosMarks) {
                if (pointOnRatioMark(ratioMark, point)) {
                    isMovingRatioMark = true;
                    movingRatioMark = &ratioMark;
                    lastDragPos = getParentComponent()->getLocalPoint(this, event.getPosition());
                    return;
                }
            }

            ratioMarkStartPos = point;
            ratioMarkLastPos = point;
            isDrawingRatioMark = true;
            return;
        }

        for (int i = int(notes.size()) - 1; i >= 0; --i) {
            juce::Path notePath = getNotePath(notes[i]);
            if (notePath.contains(point.toFloat())) {
                float noteX2 = (notes[i].time + notes[i].duration) * bar_width_px;
                if (noteX2 - point.toFloat().getX() < note_right_corner_width) {
                    if (!event.mods.isShiftDown() && !notes[i].isSelected) {
                        unselectAllNotes();
                    }
                    notes[i].isSelected = true;
                    repaint();
                    setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
                    isResizing = true;
                    lastDragPos = getParentComponent()->getLocalPoint(this, event.getPosition());
                    return;
                }
                isMoving = true;
                lastDragPos = getParentComponent()->getLocalPoint(this, event.getPosition());
                if (notes[i].isSelected) {
                    if (!event.mods.isShiftDown()) {
                        needToUnselectAllNotesExcept = i;
                    }
                } else {
                    if (!event.mods.isShiftDown()) {
                        unselectAllNotes();
                    }
                    notes[i].isSelected = true;
                    repaint();
                }
                if (params->playDraggedNotes) {
                    std::lock_guard<std::mutex> lock(mptcMtx);
                    for (const Note &note : notes) {
                        if (note.isSelected) {
                            manuallyPlayedKeysTotalCents.insert(note.cents + note.octave * 1200);
                        }
                    }
                    editor->setManuallyPlayedNotesTotalCents(manuallyPlayedKeysTotalCents);
                }
                return;
            }
        }

        bool thereAreSelNotes = thereAreSelectedNotes();
        unselectAllNotes();
        if (thereAreSelNotes) {
            editor->hideVelocityPanel();
            return;
        }

        float time = float(point.getX()) / bar_width_px;
        if (params->timeSnap) {
            time = timeToSnappedTime(time);
        }
        float duration = std::min(lastDuration, params->get_num_bars() - time);
        if (params->timeSnap) {
            duration = std::max(timeToSnappedTime(duration),
                                1.0f / (params->num_beats * params->num_subdivs));
        }
        lastDuration = duration;
        int octave, cents;
        std::tie(octave, cents) = pointToOctaveCents(point);
        if (params->keySnap && !keys.empty()) {
            std::tie(octave, cents) = centsToKeysCents(octave, cents);
        }
        notesHistory.push(notes);
        notes.push_back({octave, cents, time, false, duration, lastVelocity});
        keysFromAllNotes.insert(cents);
        if (params->zones.isNoteInActiveZone(*(notes.rbegin()))) {
            auto [_, inserted] = keys.insert(cents);
            if (inserted || keyIsGenNew[cents]) {
                keyIsGenNew[cents] = false;
                if (params->generateNewKeys) {
                    generateNewKeys();
                }
                editor->updateKeys(keys);
            }
        }
        editor->updateNotes(notes);
        repaint();

        if (params->playDraggedNotes) {
            // play placed note
            const int totalCents = cents + octave * 1200;
            {
                std::lock_guard<std::mutex> lock(mptcMtx);
                // we need to do this for playing a key that is already been played
                if (manuallyPlayedKeysTotalCents.erase(totalCents) != 0) {
                    editor->setManuallyPlayedNotesTotalCents(manuallyPlayedKeysTotalCents);
                }
                manuallyPlayedKeysTotalCents.insert(totalCents);
                editor->setManuallyPlayedNotesTotalCents(manuallyPlayedKeysTotalCents);
                placedNoteKeyCounter[totalCents]++;
            }

            auto bpmNumDenom = editor->getBpmNumDenom();
            float durationSeconds = std::get<1>(bpmNumDenom) * (60.0f / std::get<0>(bpmNumDenom)) *
                                    (4.0f / std::get<2>(bpmNumDenom)) * duration;

            juce::Timer::callAfterDelay(
                static_cast<int>(durationSeconds * 1000),
                [totalCents, safeThis = SafePointer(this)]() {
                    if (safeThis != nullptr) {
                        std::lock_guard<std::mutex> lock(safeThis->mptcMtx);
                        safeThis->placedNoteKeyCounter[totalCents]--;
                        if (safeThis->placedNoteKeyCounter[totalCents] == 0) {
                            safeThis->manuallyPlayedKeysTotalCents.erase(totalCents);
                            safeThis->editor->setManuallyPlayedNotesTotalCents(
                                safeThis->manuallyPlayedKeysTotalCents);
                        }
                    }
                });
        }
    }

    if (event.mods.isRightButtonDown()) {
        if (params->editRatiosMarks) {
            std::erase_if(params->ratiosMarks, [&](const auto &ratioMark) {
                return pointOnRatioMark(ratioMark, point);
            });
            repaint();
            return;
        }

        selectStartPos = point;
        juce::Point<float> pointFloat = event.getPosition().toFloat();
        for (int i = int(notes.size()) - 1; i >= 0; --i) {
            juce::Path notePath = getNotePath(notes[i]);
            if (notePath.contains(pointFloat)) {
                notesHistory.push(notes);
                deleteNote(i);
                editor->updateNotes(notes);
                repaint();
                return;
            }
        }
        isSelecting = true;
        selectionRect = juce::Rectangle<int>(point, point);
    }
}

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

std::pair<int, int> MainPanel::findNearestKey(int cents, int octave) {
    int minDist = 10000;
    int nearestCents;
    int nearestOctave = octave;
    for (const int key : keys) {
        if (std::abs(cents - key) < minDist) {
            minDist = std::abs(cents - key);
            nearestCents = key;
        }
    }
    if (std::abs(1200 + *keys.begin() - cents) < minDist) {
        nearestOctave += 1;
        nearestCents = *keys.begin();
    } else if (std::abs(*keys.rbegin() - 1200 - cents) < minDist) {
        nearestOctave -= 1;
        nearestCents = *keys.rbegin();
    }
    return std::make_pair(nearestCents, nearestOctave);
}

void MainPanel::mouseDrag(const juce::MouseEvent &event) {
    // Convert to parent (viewport) coordinates
    auto currentPos = getParentComponent()->getLocalPoint(this, event.getPosition());
    auto delta = currentPos - lastDragPos;
    lastDragPos = currentPos;

    if (isDragging) {
        if (params->isCamFixedOnPlayHead) {
            delta.setX(0);
        }
        if (auto *viewport = findParentComponentOfClass<juce::Viewport>()) {
            auto newPos = viewport->getViewPosition() - delta;
            lastViewPos = newPos;
            viewport->setViewPosition(newPos);
        }
        editor->repaintTopPanel();
        // repaint();
    }

    if (isResizing) {
        if (!wasResizing) {
            notesHistory.push(notes);
        }
        wasResizing = true;
        float delta_duration = float(delta.getX()) / bar_width_px;
        dtime += delta_duration;
        bool resized = false;
        float dt = 1.0f / (params->num_beats * params->num_subdivs);
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected && (notes[i].duration + delta_duration > 1.0 / 256)) {
                if (params->timeSnap) {
                    if (abs(dtime) >= dt) {
                        notes[i].duration = std::max(
                            float(notes[i].duration + sgn(dtime) * floor(abs(dtime) / dt) * dt),
                            dt);
                        resized = true;
                    }
                } else {
                    notes[i].duration += delta_duration;
                    resized = true;
                }
                lastDuration = notes[i].duration;
            }
        }
        if (abs(dtime) >= dt) {
            dtime = dtime - sgn(dtime) * floor(abs(dtime) / dt) * dt;
        }
        if (resized)
            remakeKeys();
        editor->updateKeys(keys);
        editor->updateNotes(notes);
        repaint();
    }

    if (isMovingRatioMark) {
        if (!movingRatioMark) {
            return;
        }

        float delta_time = float(delta.getX()) / bar_width_px;
        dtime += delta_time;
        bool moved = false;
        float dt = 1.0f / (params->num_beats * params->num_subdivs);

        if (movingRatioMark->time + delta_time > 0 &&
            movingRatioMark->time + delta_time < params->get_num_bars()) {
            if (params->timeSnap) {
                if (abs(dtime) >= dt) {
                    movingRatioMark->time += sgn(dtime) * floor(abs(dtime) / dt) * dt;
                    moved = true;
                }
            } else {
                moved = true;
                movingRatioMark->time += delta_time;
            }
        }

        if (abs(dtime) >= dt) {
            dtime = dtime - sgn(dtime) * floor(abs(dtime) / dt) * dt;
        }
        if (moved) {
            repaint();
        }
    }

    if (isMoving) {
        if (!wasMoving) {
            notesHistory.push(notes);
        }
        wasMoving = true;
        needToUnselectAllNotesExcept = -1;
        float delta_time = float(delta.getX()) / bar_width_px;
        dtime += delta_time;
        int delta_cents = (int)round(-delta.getY() * 1200.0 / octave_height_px);
        if (event.mods.isShiftDown()) {
            delta_cents = juce::roundFloatToInt(delta_cents * vertMoveSlowCoef);
        }
        dcents += delta_cents;
        bool moved = false;
        int numOfSelectedNotes = getNumOfSelectedNotes();
        bool updatedManuallyPlayedKeys = false;
        float dt = 1.0f / (params->num_beats * params->num_subdivs);
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected) {
                if (notes[i].time + delta_time > 0 &&
                    notes[i].time + notes[i].duration + delta_time < params->get_num_bars()) {
                    if (params->timeSnap) {
                        if (abs(dtime) >= dt) {
                            notes[i].time += sgn(dtime) * floor(abs(dtime) / dt) * dt;
                            moved = true;
                        }
                    } else {
                        moved = true;
                        notes[i].time += delta_time;
                    }
                }
                int new_cents = notes[i].cents;
                int new_octave = notes[i].octave;
                if (params->keySnap) {
                    if (numOfSelectedNotes == 1) {
                        new_cents += dcents;
                        if (new_cents >= 1200) {
                            new_octave += 1;
                            new_cents = new_cents % 1200;
                        }
                        if (new_cents < 0) {
                            new_octave -= 1;
                            new_cents = 1200 + new_cents;
                        }
                        std::tie(new_cents, new_octave) = findNearestKey(new_cents, new_octave);
                        dcents -= (new_octave * 1200 + new_cents - notes[i].octave * 1200 -
                                   notes[i].cents);
                    }
                } else {
                    new_cents += delta_cents;
                    if (new_cents >= 1200) {
                        new_octave += 1;
                        new_cents = new_cents % 1200;
                    }
                    if (new_cents < 0) {
                        new_octave -= 1;
                        new_cents = 1200 + new_cents;
                    }
                }
                if (new_octave >= 0 && new_octave < params->num_octaves) {
                    if ((new_cents != notes[i].cents) || (new_octave != notes[i].octave)) {
                        if (params->playDraggedNotes) {
                            std::lock_guard<std::mutex> lock(mptcMtx);
                            manuallyPlayedKeysTotalCents.erase(notes[i].cents +
                                                               notes[i].octave * 1200);
                            manuallyPlayedKeysTotalCents.insert(new_cents + new_octave * 1200);
                            updatedManuallyPlayedKeys = true;
                        }
                        moved = true;
                        notes[i].octave = new_octave;
                        notes[i].cents = new_cents;
                    }
                }
            }
        }
        if (abs(dtime) >= dt) {
            dtime = dtime - sgn(dtime) * floor(abs(dtime) / dt) * dt;
        }
        if (moved) {
            remakeKeys();
            editor->updateNotes(notes);
        }
        if (updatedManuallyPlayedKeys) {
            std::lock_guard<std::mutex> lock(mptcMtx);
            editor->setManuallyPlayedNotesTotalCents(manuallyPlayedKeysTotalCents);
        }
        repaint();
    }

    if (isSelecting) {
        selectionRect = juce::Rectangle<int>(selectStartPos, event.getPosition());
        repaint();
    }

    if (isDrawingRatioMark) {
        ratioMarkLastPos = event.getPosition();
        auto currentMods = juce::ModifierKeys::getCurrentModifiers();
        if (currentMods.isRightButtonDown()) {
            isDrawingRatioMark = false;
        }
        repaint();
    }
}

bool MainPanel::doesPathIntersectRect(const juce::Path &somePath,
                                      const juce::Rectangle<float> &rect) {
    if (!somePath.getBounds().intersects(rect))
        return false;

    juce::Path::Iterator iter(somePath);
    iter.next();
    auto prevPoint = juce::Point(iter.x1, iter.y1);
    while (iter.next()) {
        auto newPoint = juce::Point(iter.x1, iter.y1);
        auto line = juce::Line(prevPoint, newPoint);
        bool intersects = rect.intersects(line);
        if (intersects)
            return true;
        prevPoint = newPoint;
    }
    return false;
}

void MainPanel::mouseUp(const juce::MouseEvent &event) {
    if (isMoving && params->playDraggedNotes) {
        std::lock_guard<std::mutex> lock(mptcMtx);
        for (const Note &note : notes) {
            if (note.isSelected) {
                manuallyPlayedKeysTotalCents.erase(note.cents + note.octave * 1200);
            }
        }
        editor->setManuallyPlayedNotesTotalCents(manuallyPlayedKeysTotalCents);
    }

    isDragging = false;
    wasResizing = false;
    isResizing = false;
    isMoving = false;
    wasMoving = false;
    isMovingRatioMark = false;
    dtime = 0.0f;
    dcents = 0;
    setMouseCursor(juce::MouseCursor::NormalCursor);

    if (needToUnselectAllNotesExcept != -1) {
        unselectAllNotes();
        notes[needToUnselectAllNotesExcept].isSelected = true;
        needToUnselectAllNotesExcept = -1;
    }

    if (isSelecting) {
        if (!event.mods.isShiftDown()) {
            unselectAllNotes();
        }
        for (int i = 0; i < notes.size(); ++i) {
            juce::Path notePath = getNotePath(notes[i]);
            if (doesPathIntersectRect(notePath, selectionRect.toFloat())) {
                notes[i].isSelected = true;
            }
        }
        isSelecting = false;
    }

    if (isDrawingRatioMark) {
        if ((ratioMarkStartPos != ratioMarkLastPos) && !keys.empty()) {
            int startKeyOctave, startKeyCents, lastKeyOctave, lastKeyCents;

            std::tie(startKeyOctave, startKeyCents) = pointToOctaveCents(ratioMarkStartPos);
            std::tie(startKeyCents, startKeyOctave) = findNearestKey(startKeyCents, startKeyOctave);
            int startKeyTotalCents = startKeyOctave * 1200 + startKeyCents;

            std::tie(lastKeyOctave, lastKeyCents) = pointToOctaveCents(ratioMarkLastPos);
            std::tie(lastKeyCents, lastKeyOctave) = findNearestKey(lastKeyCents, lastKeyOctave);
            int lastKeyTotalCents = lastKeyOctave * 1200 + lastKeyCents;

            if (startKeyTotalCents != lastKeyTotalCents) {
                // let start key be lower than last key
                if (startKeyTotalCents > lastKeyTotalCents) {
                    int x = startKeyTotalCents;
                    startKeyTotalCents = lastKeyTotalCents;
                    lastKeyTotalCents = x;
                }

                float time = float(ratioMarkStartPos.getX()) / bar_width_px;
                if (params->timeSnap) {
                    time = timeToSnappedTime(time);
                }

                RatioMark ratioMark(startKeyTotalCents, lastKeyTotalCents, time, params);
                params->ratiosMarks.push_back(ratioMark);
            }
        }
        isDrawingRatioMark = false;
    }

    repaint();
}

void MainPanel::mouseMove(const juce::MouseEvent &event) {
    if (isDragging || isMoving || isResizing || isSelecting || isMovingRatioMark)
        return;

    if (params->editRatiosMarks) {
        bool isOverRatioMark = false;
        juce::Point<int> point = event.getPosition();
        for (const auto &ratioMark : params->ratiosMarks) {
            if (pointOnRatioMark(ratioMark, point)) {
                isOverRatioMark = true;
                break;
            }
        }
        if (isOverRatioMark)
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        else
            setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    juce::Point<float> point = event.getPosition().toFloat();

    bool isOverNote = false;
    bool isOverNoteResize = false;

    for (const auto &note : notes) {
        juce::Path notePath = getNotePath(note);
        if (notePath.contains(point)) {
            float noteX2 = (note.time + note.duration) * bar_width_px;
            if (noteX2 - point.getX() < note_right_corner_width)
                isOverNoteResize = true;
            else
                isOverNote = true;
            break;
        }
    }

    if (isOverNote)
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    else if (isOverNoteResize)
        setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void MainPanel::deleteNote(int i) {
    int note_cents = notes[i].cents;

    int num_notes_cents_i = 0;
    for (int j = 0; j < notes.size(); ++j) {
        if ((notes[j].cents == note_cents) && (params->zones.isNoteInActiveZone(notes[j])))
            num_notes_cents_i++;
    }

    int num_notes_cents_i_ghost_not_visible = 0;
    for (int j = 0; j < ghostNotes.size(); ++j) {
        if (ghostNotes[j].cents == note_cents) {
            if (params->showGhostNotesKeys) {
                num_notes_cents_i++;
            } else {
                num_notes_cents_i_ghost_not_visible++;
            } 
        }
    }

    notes.erase(notes.begin() + i);

    if (num_notes_cents_i == 1) {
        if (keys.erase(note_cents) > 0) {
            if (params->generateNewKeys) {
                generateNewKeys();
            }
            if (num_notes_cents_i_ghost_not_visible + num_notes_cents_i == 1) {
                keysFromAllNotes.erase(note_cents);
                reattachRatiosMarks();
            }
            editor->updateKeys(keys);
        }
    }
}

void MainPanel::generateNewKeys() {
    // Remove previously generated keys
    auto it = keys.begin();
    while (it != keys.end()) {
        if (keyIsGenNew[*it]) {
            it = keys.erase(it);
        } else {
            ++it;
        }
    }

    if (params->genNewKeysTactics == Parameters::GenNewKeysTactics::Random) {
        pickableKeys.fill(true);
        // Get rid of new possible keys that are too close to existing ones
        for (auto it = keys.begin(); it != keys.end(); ++it) {
            int ind = *it - params->minDistExistNewKeys + 1;
            int endInd = *it + params->minDistExistNewKeys - 1;
            while (ind <= endInd) {
                if (ind < 0) {
                    pickableKeys[1200 + ind] = false;
                } else if (ind >= 1200) {
                    pickableKeys[ind - 1200] = false;
                } else {
                    pickableKeys[ind] = false;
                }
                ind++;
            }
        }
        // Find new keys
        for (int i = 0; i < params->numNewGenKeys; ++i) {
            // Find random candidate
            int numPickableKeys = std::count(pickableKeys.begin(), pickableKeys.end(), true);
            if (numPickableKeys == 0)
                break;

            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(0, numPickableKeys - 1);
            int randInd = dist(gen);

            int count = 0;
            int newGenKey = 0;
            for (int i = 0; i < 1200; ++i) {
                if (pickableKeys[i]) {
                    if (count == randInd) {
                        newGenKey = i;
                        break;
                    }
                    ++count;
                }
            }

            // Get rid of new possible keys that are too close to this one
            int ind = newGenKey - params->minDistExistNewKeys + 1;
            int endInd = newGenKey + params->minDistExistNewKeys - 1;
            while (ind <= endInd) {
                if (ind < 0) {
                    pickableKeys[1200 + ind] = false;
                } else if (ind >= 1200) {
                    pickableKeys[ind - 1200] = false;
                } else {
                    pickableKeys[ind] = false;
                }
                ind++;
            }
            // Add key
            keys.insert(newGenKey);
            keyIsGenNew[newGenKey] = true;
        }
    } else if (params->genNewKeysTactics == Parameters::GenNewKeysTactics::DiverseIntervals) {
        // Init metric on intervals
        intervalsWere.fill(false);
        intervalsDist.fill(100000);
        for (auto it1 = keys.begin(); it1 != keys.end(); ++it1) {
            auto it2 = it1;
            ++it2;
            for (; it2 != keys.end(); ++it2) {
                int interval1 = (*it2 - *it1) % 1200;
                int interval2 = 1200 - interval1;
                if (!intervalsWere[interval1]) {
                    for (int i = 0; i < 1200; ++i) {
                        intervalsDist[i] = std::min(intervalsDist[i], abs(interval1 - i));
                    }
                    intervalsWere[interval1] = true;
                }
                if (!intervalsWere[interval2]) {
                    for (int i = 0; i < 1200; ++i) {
                        // not very effective
                        intervalsDist[i] = std::min(intervalsDist[i], abs(interval2 - i));
                    }
                    intervalsWere[interval2] = true;
                }
            }
        }

        // Find interval weights of each possible new key
        for (int i = 0; i < 120; ++i) {
            possibleNewKeysWeights[i] = 1;
            for (auto it = keys.begin(); it != keys.end(); ++it) {
                int interval1 = abs(*it - i * 10) % 1200;
                possibleNewKeysWeights[i] += intervalsDist[interval1];
                int interval2 = 1200 - interval1;
                if (interval2 != 1200)
                    possibleNewKeysWeights[i] += intervalsDist[interval2];
            }
        }

        // Get rid of new possible keys that are too close to existing ones
        for (auto it = keys.begin(); it != keys.end(); ++it) {
            int ind = int(ceil((*it - params->minDistExistNewKeys + 1) / 10.0));
            int endInd = (*it + params->minDistExistNewKeys - 1) / 10;
            while (ind <= endInd) {
                if (ind < 0) {
                    possibleNewKeysWeights[120 + ind] = -1;
                } else if (ind >= 120) {
                    possibleNewKeysWeights[ind - 120] = -1;
                } else {
                    possibleNewKeysWeights[ind] = -1;
                }
                ind++;
            }
        }

        // Find new keys
        for (int i = 0; i < params->numNewGenKeys; ++i) {
            // Find best candidate
            int bestInd = 0;
            float bestWeight = (possibleNewKeysWeights[0]);
            for (int j = 1; j < 120; ++j) {
                float weight = (possibleNewKeysWeights[j]);
                if (weight > bestWeight) {
                    bestWeight = weight;
                    bestInd = j;
                }
            }
            int newGenKey = 10 * bestInd;
            // Get rid of new possible keys that are too close to this one
            int ind = int(ceil((newGenKey - params->minDistBetweenNewKeys + 1) / 10.0));
            int endInd = (newGenKey + params->minDistBetweenNewKeys - 1) / 10;
            while (ind <= endInd) {
                if (ind < 0) {
                    possibleNewKeysWeights[120 + ind] = -1;
                } else if (ind >= 120) {
                    possibleNewKeysWeights[ind - 120] = -1;
                } else {
                    possibleNewKeysWeights[ind] = -1;
                }
                ind++;
            }
            // Add key
            keys.insert(newGenKey);
            keyIsGenNew[newGenKey] = true;
        }
    }
}

std::optional<int> MainPanel::findNearestKeyWithLimit(int key, int maxCentsChange,
                                                      const std::set<int> &keys) {
    if (keys.empty()) {
        return std::nullopt;
    }

    int bestKey = -1;
    int bestDistance = maxCentsChange + 1;

    for (const auto k : keys) {
        int diff = std::abs(k - key);
        int dist = std::min(diff, 1200 - diff);
        if ((dist != 0) && (dist < bestDistance)) {
            bestKey = k;
            bestDistance = dist;
        }
    }

    if (bestDistance <= maxCentsChange) {
        return bestKey;
    }
    return std::nullopt;
}

void MainPanel::reattachRatiosMarks() {
    if (params->autoCorrectRatiosMarks) {
        const int maxCentsChange = 100;
        for (auto &ratioMark : params->ratiosMarks) {
            int higherKeyCents = ratioMark.getHigherKeyTotalCents() % 1200;
            if (!keysFromAllNotes.contains(higherKeyCents)) {
                auto result = findNearestKeyWithLimit(higherKeyCents, maxCentsChange, keysFromAllNotes);
                if (result) {
                    int higherKeyOctave = ratioMark.getHigherKeyTotalCents() / 1200;
                    int newHigherKeyCents = *result;
                    if ((newHigherKeyCents - higherKeyCents < -maxCentsChange) &&
                        (higherKeyOctave < params->num_octaves - 1)) {
                        ratioMark.setHigherKeyTotalCents(1200 * (higherKeyOctave + 1) +
                                                         newHigherKeyCents);
                    } else if ((newHigherKeyCents - higherKeyCents > maxCentsChange) &&
                               (higherKeyOctave > 0)) {
                        ratioMark.setHigherKeyTotalCents(1200 * (higherKeyOctave - 1) +
                                                         newHigherKeyCents);
                    } else {
                        ratioMark.setHigherKeyTotalCents(1200 * higherKeyOctave +
                                                         newHigherKeyCents);
                    }
                }
            }
            int lowerKeyCents = ratioMark.getLowerKeyTotalCents() % 1200;
            if (!keysFromAllNotes.contains(lowerKeyCents)) {
                auto result = findNearestKeyWithLimit(lowerKeyCents, maxCentsChange, keysFromAllNotes);
                if (result) {
                    int lowerKeyOctave = ratioMark.getLowerKeyTotalCents() / 1200;
                    int newLowerKeyCents = *result;
                    if ((newLowerKeyCents - lowerKeyCents < -maxCentsChange) &&
                        (lowerKeyOctave < params->num_octaves - 1)) {
                        ratioMark.setLowerKeyTotalCents(1200 * (lowerKeyOctave + 1) +
                                                        newLowerKeyCents);
                    } else if ((newLowerKeyCents - lowerKeyCents > maxCentsChange) &&
                               (lowerKeyOctave > 0)) {
                        ratioMark.setLowerKeyTotalCents(1200 * (lowerKeyOctave - 1) +
                                                        newLowerKeyCents);
                    } else {
                        ratioMark.setLowerKeyTotalCents(1200 * lowerKeyOctave + newLowerKeyCents);
                    }
                }
            }
        }
    }
}

void MainPanel::remakeKeys() {
    keys.clear();
    keysFromAllNotes.clear();
    keyIsGenNew.fill(false);
    for (const Note &note : notes) {
        if (params->zones.isNoteInActiveZone(note)) {
            keys.insert(note.cents);
        }
        keysFromAllNotes.insert(note.cents);
    }
    if (params->showGhostNotesKeys) {
        for (const Note &note : ghostNotes) {
            if (params->zones.isNoteInActiveZone(note)) {
                keys.insert(note.cents);
            }
        }
    }
    for (const Note &note : ghostNotes) {
        keysFromAllNotes.insert(note.cents);
    }

    if (params->generateNewKeys) {
        generateNewKeys();
    }

    editor->updateKeys(keys);
    reattachRatiosMarks();
}

void MainPanel::updateRatiosMarks() {
    for (auto &ratioMark : params->ratiosMarks) {
        ratioMark.calculateRatioAndError();
    }
    repaint();
}

int python_mod(int a, int b) {
    int result = a % b;
    return result >= 0 ? result : result + std::abs(b);
}

bool MainPanel::keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) {
    juce::ignoreUnused(originatingComponent);

    // select all notes
    if (key == juce::KeyPress('a', juce::ModifierKeys::ctrlModifier, 0)) {
        selectAllNotes();
        return true;
    }

    // unselect all notes
    if (key == juce::KeyPress::escapeKey) {
        unselectAllNotes();
        editor->hideVelocityPanel();
        return true;
    }

    // delete selected notes
    if (key == juce::KeyPress::deleteKey) {
        notesHistory.push(notes);
        int numNotes = int(notes.size());
        for (int i = 0; i < numNotes; ++i) {
            if (notes[i].isSelected) {
                deleteNote(i);
                i--;
                numNotes--;
            }
        }
        editor->updateNotes(notes);
        repaint();
        return true;
    }

    // copy selected notes
    if (key == juce::KeyPress('c', juce::ModifierKeys::ctrlModifier, 0)) {
        copiedNotes.clear();
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected)
                copiedNotes.push_back(notes[i]);
        }
        editor->showMessage(juce::String(copiedNotes.size()) + " note" +
                            (copiedNotes.size() == 1 ? "" : "s") + " copied!");
        return true;
    }

    // paste copied notes
    if (key == juce::KeyPress('v', juce::ModifierKeys::ctrlModifier, 0)) {
        notesHistory.push(notes);
        unselectAllNotes();
        editor->hideVelocityPanel();
        for (int i = 0; i < copiedNotes.size(); ++i) {
            notes.push_back(copiedNotes[i]);
            int cents = copiedNotes[i].cents;
            keys.insert(cents);
            keysFromAllNotes.insert(cents);
            keyIsGenNew[cents] = false;
        }
        if (params->generateNewKeys) {
            generateNewKeys();
        }
        editor->updateKeys(keys);
        editor->showMessage(juce::String(copiedNotes.size()) + " note" +
                            (copiedNotes.size() == 1 ? "" : "s") + " pasted!");
        editor->updateNotes(notes);
        repaint();
        return true;
    }

    // reverse previous state
    if (key == juce::KeyPress('z', juce::ModifierKeys::ctrlModifier, 0)) {
        if (!notesHistory.empty()) {
            notes = notesHistory.top();
            notesHistory.pop();
            unselectAllNotes();
            editor->hideVelocityPanel();
            remakeKeys();
            editor->updateNotes(notes);
            repaint();
        }
        return true;
    }

    // cents or ratio input
    if (thereAreSelectedNotes()) {
        const juce::juce_wchar c = key.getTextCharacter();
        if ((c >= '0' && c <= '9') || c == '/') {
            const juce::String &text = editor->getTextFromMessage();
            // check if there is already not cents/ratio in message
            if (!text.containsOnly("0123456789/"))
                editor->showMessage(juce::String::charToString(c));
            else
                editor->showMessage(text + juce::String::charToString(c));
            return true;
        }
        if (key == juce::KeyPress::backspaceKey) {
            const juce::String &text = editor->getTextFromMessage();
            // check if there is already not cents/ratio in message
            if (!text.containsOnly("0123456789/"))
                editor->showMessage("");
            else if (!text.isEmpty())
                editor->showMessage(text.substring(0, text.length() - 1));
            return true;
        }
    }

    // set cents for selected notes
    if (key == juce::KeyPress::returnKey) {
        notesHistory.push(notes);
        int cents = getCentsFromMessage();
        if (cents != -1) {
            cents = cents % 1200;
            for (int i = 0; i < notes.size(); ++i) {
                if (notes[i].isSelected) {
                    notes[i].cents = cents;
                }
            }
            remakeKeys();
            editor->updateNotes(notes);
            repaint();
        }
        return true;
    }

    // increase the pitch of selected notes by cents
    if (key == juce::KeyPress(juce::KeyPress::returnKey, juce::ModifierKeys::shiftModifier, 0)) {
        notesHistory.push(notes);
        int cents = getCentsFromMessage();
        if (cents != -1) {
            for (int i = 0; i < notes.size(); ++i) {
                if (notes[i].isSelected) {
                    int note_octave = notes[i].octave;
                    int note_cents = notes[i].cents;
                    note_cents = note_cents + cents;
                    note_octave += note_cents / 1200;
                    note_cents = note_cents % 1200;
                    if (note_octave > params->num_octaves - 1) {
                        note_octave = params->num_octaves - 1;
                        note_cents = 1199;
                    }
                    notes[i].octave = note_octave;
                    notes[i].cents = note_cents;
                }
            }
            remakeKeys();
            editor->updateNotes(notes);
            repaint();
        }
        return true;
    }

    // decrease the pitch of selected notes by cents
    if (key == juce::KeyPress(juce::KeyPress::returnKey, juce::ModifierKeys::ctrlModifier, 0)) {
        notesHistory.push(notes);
        int cents = getCentsFromMessage();
        if (cents != -1) {
            for (int i = 0; i < notes.size(); ++i) {
                if (notes[i].isSelected) {
                    int note_octave = notes[i].octave;
                    int note_cents = notes[i].cents;
                    note_cents = note_cents - cents;
                    note_octave += note_cents / 1200;
                    if (note_cents < 0)
                        note_octave--;
                    note_cents = python_mod(note_cents, 1200);
                    if (note_octave < 0) {
                        note_octave = 0;
                        note_cents = 0;
                    }
                    notes[i].octave = note_octave;
                    notes[i].cents = note_cents;
                }
            }
            remakeKeys();
            editor->updateNotes(notes);
            repaint();
        }
        return true;
    }

    // moving notes horizontally
    if ((key == juce::KeyPress::rightKey) ||
        (key == juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::shiftModifier, 0))) {
        if (!thereAreSelectedNotes()) {
            return false;
        }
        float dMoveTime = 1.0f / (params->num_beats * params->num_subdivs);
        if (key.getModifiers().isShiftDown()) {
            dMoveTime = 1.0f;
        }
        int numBars = params->get_num_bars();
        notesHistory.push(notes);
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected) {
                if (notes[i].time + notes[i].duration + dMoveTime <= numBars) {
                    notes[i].time += dMoveTime;
                }
            }
        }
        remakeKeys(); // because notes can enter/leave time active/disabled zones
        editor->updateNotes(notes);
        repaint();
        return true;
    }
    if ((key == juce::KeyPress::leftKey) ||
        (key == juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::shiftModifier, 0))) {
        if (!thereAreSelectedNotes()) {
            return false;
        }
        float dMoveTime = 1.0f / (params->num_beats * params->num_subdivs);
        if (key.getModifiers().isShiftDown()) {
            dMoveTime = 1.0f;
        }
        int numBars = params->get_num_bars();
        notesHistory.push(notes);
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected) {
                if (notes[i].time - dMoveTime >= 0) {
                    notes[i].time -= dMoveTime;
                }
            }
        }
        remakeKeys(); // because notes can enter/leave active/disabled time zones
        editor->updateNotes(notes);
        repaint();
        return true;
    }

    // raising or lowering selected notes by a cent (or by a key in key snap mode)
    if (key == juce::KeyPress::upKey) {
        if (!thereAreSelectedNotes()) {
            return false;
        }
        notesHistory.push(notes);
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected) {
                if (params->keySnap) {
                    if (!((notes[i].octave == params->num_octaves - 1) &&
                          (notes[i].cents == *keys.rbegin()))) {
                        int keyInd = getKeyIndex(notes[i].cents) + 1;
                        if (keyInd == keys.size()) {
                            notes[i].octave++;
                            notes[i].cents = *keys.begin();
                        } else {
                            notes[i].cents = *(std::next(keys.begin(), keyInd));
                        }
                    }
                } else {
                    if (notes[i].cents == 1199) {
                        if (notes[i].octave == params->num_octaves - 1)
                            continue;
                        else
                            notes[i].octave++;
                        notes[i].cents = 0;
                    } else {
                        notes[i].cents++;
                    }
                }
            }
        }
        remakeKeys();
        editor->updateNotes(notes);
        repaint();
        return true;
    }
    if (key == juce::KeyPress::downKey) {
        if (!thereAreSelectedNotes()) {
            return false;
        }
        notesHistory.push(notes);
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected) {
                if (params->keySnap) {
                    if (!((notes[i].octave == 0) && (notes[i].cents == *keys.begin()))) {
                        int keyInd = getKeyIndex(notes[i].cents) - 1;
                        if (keyInd == -1) {
                            notes[i].octave--;
                            notes[i].cents = *keys.rbegin();
                        } else {
                            notes[i].cents = *(std::next(keys.begin(), keyInd));
                        }
                    }
                } else {
                    if (notes[i].cents == 0) {
                        if (notes[i].octave == 0)
                            continue;
                        else
                            notes[i].octave--;
                        notes[i].cents = 1199;
                    } else {
                        notes[i].cents--;
                    }
                }
            }
        }
        remakeKeys();
        editor->updateNotes(notes);
        repaint();
        return true;
    }

    // raising or lowering selected notes by an octave
    if (key == juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::shiftModifier, 0)) {
        notesHistory.push(notes);
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected && (notes[i].octave != params->num_octaves - 1))
                notes[i].octave += 1;
        }
        editor->updateNotes(notes);
        repaint();
        return true;
    }
    if (key == juce::KeyPress(juce::KeyPress::downKey, juce::ModifierKeys::shiftModifier, 0)) {
        notesHistory.push(notes);
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected && (notes[i].octave != 0))
                notes[i].octave -= 1;
        }
        editor->updateNotes(notes);
        repaint();
        return true;
    }

    // HOTKEYS
    // click timeSnapButton
    if (key ==
        juce::KeyPress(Parameters::hotkeys::timeSnap_withAlt, juce::ModifierKeys::altModifier, 0)) {
        editor->leftClickTimeSnapButton();
        return true;
    }
    // click keySnapButton
    if (key ==
        juce::KeyPress(Parameters::hotkeys::keySnap_withAlt, juce::ModifierKeys::altModifier, 0)) {
        editor->leftClickKeySnapButton();
        return true;
    }
    // click editRatiosMarksButton
    if (key == juce::KeyPress(Parameters::hotkeys::editRatiosMarks_withAlt,
                              juce::ModifierKeys::altModifier, 0)) {
        editor->leftClickEditRatiosMarksButton();
        return true;
    }
    // click pitchMemoryButton
    if (key == juce::KeyPress(Parameters::hotkeys::pitchMemory_withAlt,
                              juce::ModifierKeys::altModifier, 0)) {
        editor->leftClickPitchMemoryButton();
        return true;
    }

    // playing a key
    auto keyChar = juce::CharacterFunctions::toLowerCase(key.getTextCharacter());
    int keyInd = keysPlaySet.indexOfChar(keyChar);
    if (keyInd != -1) {
        int numKeys = int(keys.size());
        if ((numKeys != 0) && !wasKeyDown.contains(keyChar)) {
            int octave = params->start_octave + keyInd / numKeys;
            int cents = *(std::next(keys.begin(), keyInd % numKeys));
            int totalCents = octave * 1200 + cents;
            std::lock_guard<std::mutex> lock(mptcMtx);
            manuallyPlayedKeysTotalCents.insert(totalCents);
            editor->setManuallyPlayedNotesTotalCents(manuallyPlayedKeysTotalCents);
            wasKeyDown[keyChar] = totalCents;
        }
        return true;
    }

    return false;
}

bool MainPanel::keyStateChanged(bool isKeyDown) {
    if (!isKeyDown) {
        // stop playing a key if it is playing
        bool werePlaying = false;
        for (int i = 0; i < keysPlaySet.length(); ++i) {
            int keyCode =
                juce::KeyPress::createFromDescription(keysPlaySet.substring(i, i + 1)).getKeyCode();
            auto keyChar = keysPlaySet[i];
            if (!juce::KeyPress::isKeyCurrentlyDown(keyCode) && wasKeyDown.contains(keyChar)) {
                // Key may change while pressing key, so use stored value in wasKeyDown
                /*int octave = params->start_octave + i / numKeys;
                int cents = *(std::next(keys.begin(), i % numKeys));
                int totalCents = octave * 1200 + cents;*/
                int totalCents = wasKeyDown[keyChar];
                std::lock_guard<std::mutex> lock(mptcMtx);
                manuallyPlayedKeysTotalCents.erase(totalCents);
                editor->setManuallyPlayedNotesTotalCents(manuallyPlayedKeysTotalCents);
                wasKeyDown.erase(keyChar);
                werePlaying = true;
            }
        }
        return werePlaying;
    }

    return false;
}

int countSlashes(const juce::String &str) {
    int count = 0;
    for (juce::juce_wchar c : str) {
        if (c == '/')
            count++;
    }
    return count;
}

int MainPanel::getCentsFromMessage() {
    const juce::String text = editor->getTextFromMessage();
    editor->hideMessage();

    if (!text.isEmpty() && text.containsOnly("0123456789/")) {
        const int numSlashes = countSlashes(text);
        if (numSlashes == 0) {
            return text.getIntValue();
        } else if (numSlashes == 1) {
            int slashPos = text.indexOfChar(juce::juce_wchar('/'));
            int num = text.substring(0, slashPos).getIntValue();
            int den = text.substring(slashPos + 1).getIntValue();
            return ((den != 0) && (num >= den)) ? (int)round(1200 * log2(float(num) / den)) : -1;
        }
    }

    return -1;
}

void MainPanel::selectAllNotes() {
    for (int i = 0; i < notes.size(); ++i)
        notes[i].isSelected = true;
    repaint();
}

void MainPanel::updateLayout() {
    this->setSize(params->get_num_bars() * bar_width_px, params->num_octaves * octave_height_px);
    repaint();
}

bool MainPanel::thereAreSelectedNotes() {
    for (const Note &note : notes) {
        if (note.isSelected) {
            return true;
        }
    }
    return false;
}

int MainPanel::getNumOfSelectedNotes() {
    int num = 0;
    for (const Note &note : notes) {
        if (note.isSelected) {
            num++;
        }
    }
    return num;
}

juce::Path MainPanel::getNotePath(const Note &note) {
    float height = octave_height_px * params->noteRectHeightCoef;
    float width = bar_width_px * note.duration;
    float x1 = note.time * bar_width_px;
    float x2 = x1 + width;
    float y1 = (params->num_octaves - note.octave - float(note.cents) / 1200) * octave_height_px;
    float dy = float(note.bend) / 1200 * octave_height_px;
    float y2 = y1 - dy;

    juce::Path path;
    if (note.bend == 0) {
        // make horizontal rectangle
        path.startNewSubPath(x1, y1 - height / 2);
        path.lineTo(x2, y2 - height / 2);
        path.lineTo(x2, y2 + height / 2);
        path.lineTo(x1, y1 + height / 2);
        path.closeSubPath();
    } else if (height >= width) {
        // make vertical rectangle
        path.startNewSubPath(x1, y1 - height / 2);
        path.lineTo(x2, y1 - height / 2);
        path.lineTo(x2, y2 + height / 2);
        path.lineTo(x1, y2 + height / 2);
        path.closeSubPath();
    } else {
        // make hexagon
        if (dy > 0) {
            float z = height * (sqrt(width * width + dy * dy - 2 * height * dy) - width) /
                      (dy - 2 * height);
            path.startNewSubPath(x1, y1 - height / 2);
            path.lineTo(x2 - z, y2 - height / 2);
            path.lineTo(x2, y2 - height / 2);
            path.lineTo(x2, y2 + height / 2);
            path.lineTo(x1 + z, y1 + height / 2);
            path.lineTo(x1, y1 + height / 2);
            path.closeSubPath();
        } else {
            dy = -dy;
            float z = height * (sqrt(width * width + dy * dy - 2 * height * dy) - width) /
                      (dy - 2 * height);
            path.startNewSubPath(x1, y1 - height / 2);
            path.lineTo(x1 + z, y1 - height / 2);
            path.lineTo(x2, y2 - height / 2);
            path.lineTo(x2, y2 + height / 2);
            path.lineTo(x2 - z, y2 + height / 2);
            path.lineTo(x1, y1 + height / 2);
            path.closeSubPath();
        }
    }

    return path;
}

float MainPanel::timeToSnappedTime(float time) {
    return 1.0f / (params->num_beats * params->num_subdivs) *
           floor(time / (1.0f / (params->num_beats * params->num_subdivs)));
}

std::tuple<int, int> MainPanel::centsToKeysCents(int octave, int cents) {
    int nearest = 0;
    int minDist = 100000;
    for (const int &key : keys) {
        if (abs(key - cents) < minDist) {
            minDist = abs(key - cents);
            nearest = key;
        }
    }
    if (octave != 0 && abs(1200 + cents - *keys.rbegin()) < minDist) {
        octave--;
        nearest = *keys.rbegin();
    }
    if (octave != params->num_octaves + 1 && abs(1200 + *keys.begin() - cents) < minDist) {
        octave++;
        nearest = *keys.begin();
    }
    return {octave, nearest};
}

int MainPanel::getKeyIndex(int cents) {
    auto it = keys.find(cents);
    if (it != keys.end()) {
        return int(std::distance(keys.begin(), it));
    }
    return -1;
}

void MainPanel::setPlayHeadTime(float newPlayHeadTime) {
    playHeadTime = newPlayHeadTime;
    repaint();
}

void MainPanel::updateNotes(const std::vector<Note> &new_notes) {
    notes = new_notes;
    remakeKeys();
    repaint();
}

void MainPanel::updateGhostNotes(const std::vector<Note> &new_ghostNotes) {
    if (ghostNotes == new_ghostNotes)
        return;
    ghostNotes = new_ghostNotes;
    if (params->showGhostNotesKeys) {
        remakeKeys();
    } else {
        keysFromAllNotes.clear();
        for (const Note &note : notes) {
            keysFromAllNotes.insert(note.cents);
        }
        for (const Note &note : ghostNotes) {
            keysFromAllNotes.insert(note.cents);
        }
    }
    repaint();
}

void MainPanel::setVelocitiesOfSelectedNotes(float vel) {
    for (Note &note : notes) {
        if (note.isSelected) {
            note.velocity = vel;
        }
    }
    lastVelocity = vel;
    editor->updateNotes(notes);
    repaint();
}
} // namespace audio_plugin