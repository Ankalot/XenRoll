#include "XenRoll/PluginEditor.h" // must be first!
#include "XenRoll/MainPanel.h"    // must be second!

namespace audio_plugin {
MainPanel::MainPanel(int octave_height_px, int bar_width_px,
                     AudioPluginAudioProcessorEditor *editor, Parameters *params)
    : octave_height_px(octave_height_px), bar_width_px(bar_width_px), editor(editor),
      params(params) {
    this->setSize(params->get_num_bars() * bar_width_px, params->num_octaves * octave_height_px);
    setInterceptsMouseClicks(true, true);

    addKeyListener(this);
    setWantsKeyboardFocus(true);

    notes = editor->getNotes();
    unselectAllNotes();
    remakeKeys();

    bendFont = juce::Font(getLookAndFeel().getTypefaceForFont(NULL)).withPointHeight(Theme::small_);
}

MainPanel::~MainPanel() { removeKeyListener(this); }

int MainPanel::totalCentsToY(int totalCents) {
    return juce::roundToInt(octave_height_px * (params->num_octaves - totalCents / 1200.0));
}

void MainPanel::updatePitchMemoryResults(const PitchMemoryResults &newPitchMemoryResults) {
    pitchMemoryResults = newPitchMemoryResults;
    repaint();
}

void MainPanel::drawOutlinedText(juce::Graphics &g, const juce::String &text,
                                 juce::Rectangle<float> area, const juce::Font &font,
                                 float outlineThickness) {
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, text, area.getX(), area.getY() + font.getHeight());
    juce::Path textPath;
    glyphs.createPath(textPath);

    // Center the path in the provided area
    textPath.applyTransform(
        juce::AffineTransform::translation(area.getCentreX() - textPath.getBounds().getCentreX(),
                                           area.getCentreY() - textPath.getBounds().getCentreY()));

    // === Draw the darkest outline ===
    g.setColour(Theme::darkest);
    juce::PathStrokeType stroke(outlineThickness);
    stroke.setJointStyle(juce::PathStrokeType::mitered);
    g.strokePath(textPath, stroke);

    // === Draw the brightest fill ===
    g.setColour(Theme::brightest);
    g.fillPath(textPath);
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
        g.drawLine(float(clipX), yPos, float(clipX + clipWidth), yPos, Theme::wide);
    }
    // bars
    int bar_i_start = clipX / bar_width_px;
    int bar_i_end = std::min((clipX + clipWidth) / bar_width_px + 1, params->get_num_bars());
    for (int i = bar_i_start; i <= bar_i_end; ++i) {
        float xPos = float(i * bar_width_px);
        g.drawLine(xPos, 0.0f, xPos, float(octave_height_px * params->num_octaves), Theme::wide);
    }
    // beats and subdivisions
    for (int i = bar_i_start; i < bar_i_end; ++i) {
        for (int j = 0; j < params->num_beats; ++j) {
            float xPos = (i + float(j) / params->num_beats) * bar_width_px;
            g.drawLine(xPos, 0.0f, xPos, float(octave_height_px * params->num_octaves),
                       Theme::narrow);
            for (int k = 1; k < params->num_subdivs; ++k) {
                float xPosSub =
                    xPos + float(k) / (params->num_subdivs * params->num_beats) * bar_width_px;
                g.drawLine(xPosSub, 0.0f, xPosSub, float(octave_height_px * params->num_octaves),
                           Theme::narrower);
            }
        }
    }
    // keys
    for (const int &key : keys) {
        for (int j = octave_i_start; j < octave_i_end; ++j) {
            float yPos = (j + 1.0f - float(key) / 1200) * octave_height_px;
            g.drawLine(0.0f, yPos, float(params->get_num_bars() * bar_width_px), yPos,
                       Theme::narrow);
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
    for (const Note &note : notes) {
        juce::Path notePath = getNotePath(note);
        if (notePath.getBounds().intersects(clipFloat)) {
            if ((note.bend != 0) && note.isSelected) {
                juce::String bendText =
                    juce::String::formatted("%s%d", (note.bend > 0 ? "+" : "-"), abs(note.bend));
                drawOutlinedText(g, bendText, notePath.getBounds(), bendFont, Theme::wide);
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
                        g.drawLine(posXstart, posY, posXend, posY, Theme::wider);
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
}

void MainPanel::unselectAllNotes() {
    for (int i = 0; i < notes.size(); ++i) {
        notes[i].isSelected = false;
    }
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
            static_cast<int>(round(viewport->getHeight() / params->num_octaves)) + 1;
        octave_height_px =
            juce::jlimit(min_octave_height_px, 15 * min_octave_height_px, octave_height_px);
        editor->changeOctaveHeightPx(octave_height_px);
    } else {
        bar_width_px = static_cast<int>(round(bar_width_px * stretchFactor));
        int min_bar_width_px =
            static_cast<int>(round(viewport->getWidth() / params->get_num_bars())) + 1;
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

void MainPanel::mouseDown(const juce::MouseEvent &event) {
    grabKeyboardFocus();
    juce::Point<int> point = event.getPosition();

    if (event.mods.isMiddleButtonDown()) {
        for (int i = int(notes.size()) - 1; i >= 0; --i) {
            juce::Path notePath = getNotePath(notes[i]);
            if (notePath.contains(point.toFloat())) {
                notes[i].isSelected = true;
                editor->showVelocityPanel(notes[i].velocity);
                repaint();
                return;
            }
        }

        isDragging = true;
        // Get position in parent (viewport) coordinates
        lastDragPos = getParentComponent()->getLocalPoint(this, event.getPosition());
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }

    if (event.mods.isLeftButtonDown()) {
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
                    for (const Note& note: notes) {
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
        int octave = params->num_octaves - 1 - point.getY() / octave_height_px;
        int cents =
            static_cast<int>(round(
                (1.0f - (point.getY() % octave_height_px) * 1.0f / octave_height_px) * 1200)) %
            1200;
        if (cents == 0) {
            octave += 1;
            if (octave == params->num_octaves) {
                return;
            }
        }
        if (params->keySnap && !keys.empty()) {
            std::tie(octave, cents) = centsToKeysCents(octave, cents);
        }
        notesHistory.push(notes);
        notes.push_back({octave, cents, time, false, duration, 100.0f / 127});
        if (params->zones.isNoteInActiveZone(*(notes.rbegin()))) {
            auto [_, inserted] = keys.insert(cents);
            if (inserted) {
                editor->updateKeys(keys);
            }
        }
        editor->updateNotes(notes);
        repaint();
    }

    if (event.mods.isRightButtonDown()) {
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
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected && (notes[i].duration + delta_duration > 1.0 / 256)) {
                if (params->timeSnap) {
                    if (abs(dtime) >= 1.0f / (params->num_beats * params->num_subdivs)) {
                        notes[i].duration =
                            std::max(notes[i].duration + float(sgn(dtime)) / (params->num_beats *
                                                                              params->num_subdivs),
                                     1.0f / (params->num_beats * params->num_subdivs));
                        resized = true;
                    }
                } else {
                    notes[i].duration += delta_duration;
                    resized = true;
                }
                lastDuration = notes[i].duration;
            }
        }
        if (abs(dtime) >= 1.0f / (params->num_beats * params->num_subdivs)) {
            dtime = dtime - float(sgn(dtime)) / (params->num_beats * params->num_subdivs);
        }
        if (resized)
            remakeKeys();
        editor->updateKeys(keys);
        editor->updateNotes(notes);
        repaint();
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
            delta_cents = juce::roundFloatToInt(delta_cents*vertMoveSlowCoef);
        }
        dcents += delta_cents;
        bool moved = false;
        int numOfSelectedNotes = getNumOfSelectedNotes();
        bool updatedManuallyPlayedKeys = false;
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected) {
                if (notes[i].time + delta_time > 0 &&
                    notes[i].time + notes[i].duration + delta_time < params->get_num_bars()) {
                    if (params->timeSnap) {
                        if (abs(dtime) >= 1.0f / (params->num_beats * params->num_subdivs)) {
                            notes[i].time +=
                                float(sgn(dtime)) / (params->num_beats * params->num_subdivs);
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
                            manuallyPlayedKeysTotalCents.erase(notes[i].cents + notes[i].octave*1200);
                            manuallyPlayedKeysTotalCents.insert(new_cents + new_octave*1200);
                            updatedManuallyPlayedKeys = true;
                        }
                        moved = true;
                        notes[i].octave = new_octave;
                        notes[i].cents = new_cents;
                        remakeKeys();
                    }
                }
            }
        }
        if (abs(dtime) >= 1.0f / (params->num_beats * params->num_subdivs)) {
            dtime = dtime - float(sgn(dtime)) / (params->num_beats * params->num_subdivs);
        }
        if (moved)
            editor->updateNotes(notes);
        if (updatedManuallyPlayedKeys)
            editor->setManuallyPlayedNotesTotalCents(manuallyPlayedKeysTotalCents);
        repaint();
    }

    if (isSelecting) {
        selectionRect = juce::Rectangle<int>(selectStartPos, event.getPosition());
        repaint();
    }
}

bool MainPanel::doesPathIntersectRect(const juce::Path &parallelogram,
                                      const juce::Rectangle<float> &rect) {
    if (!parallelogram.getBounds().intersects(rect))
        return false;

    juce::Path::Iterator iter(parallelogram);
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
        for (const Note& note: notes) {
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
    repaint();
}

void MainPanel::mouseMove(const juce::MouseEvent &event) {
    if (isDragging || isMoving || isResizing || isSelecting)
        return;

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
        if (notes[j].cents == note_cents)
            num_notes_cents_i++;
    }
    
    if (params->showGhostNotesKeys) {
        for (int j = 0; j < ghostNotes.size(); ++j) {
            if (ghostNotes[j].cents == note_cents)
                num_notes_cents_i++;
        }
    }

    notes.erase(notes.begin() + i);

    if (num_notes_cents_i == 1) {
        if (keys.erase(note_cents) > 0) {
            editor->updateKeys(keys);
        }
    }
}

void MainPanel::remakeKeys() {
    std::set<int> newKeys;
    for (const Note &note : notes) {
        if (params->zones.isNoteInActiveZone(note)) {
            newKeys.insert(note.cents);
        }
    }
    if (params->showGhostNotesKeys) {
        for (const Note &note : ghostNotes) {
            if (params->zones.isNoteInActiveZone(note)) {
                newKeys.insert(note.cents);
            }
        }
    }
    keys = std::move(newKeys);
    editor->updateKeys(keys);
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

    // raising or lowering selected notes by a cent (or by a key in key snap mode)
    if (key == juce::KeyPress::upKey) {
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

    // playing a key
    auto keyChar = juce::CharacterFunctions::toLowerCase(key.getTextCharacter());
    int keyInd = keysPlaySet.indexOfChar(keyChar);
    if (keyInd != -1) {
        int octave = params->start_octave + keyInd / int(keys.size());
        int cents = *(std::next(keys.begin(), keyInd % keys.size()));
        int totalCents = octave * 1200 + cents;
        manuallyPlayedKeysTotalCents.insert(totalCents);
        editor->setManuallyPlayedNotesTotalCents(manuallyPlayedKeysTotalCents);
        wasKeyDown.insert(key.getKeyCode());
        return true;
    }

    return false;
}

bool MainPanel::keyStateChanged(bool isKeyDown) {
    if (!isKeyDown) {
        // stop playing a key if it is playing
        bool werePlaying = false;
        int numKeys = int(keys.size());
        for (int i = 0; i < keysPlaySet.length(); ++i) {
            int keyCode =
                juce::KeyPress::createFromDescription(keysPlaySet.substring(i, i + 1)).getKeyCode();
            if (!juce::KeyPress::isKeyCurrentlyDown(keyCode) && wasKeyDown.contains(keyCode)) {
                int octave = params->start_octave + i / numKeys;
                int cents = *(std::next(keys.begin(), i % numKeys));
                int totalCents = octave * 1200 + cents;
                manuallyPlayedKeysTotalCents.erase(totalCents);
                editor->setManuallyPlayedNotesTotalCents(manuallyPlayedKeysTotalCents);
                wasKeyDown.erase(keyCode);
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
    if ((note.bend == 0) || (height >= width)) {
        // make parallelogram
        path.startNewSubPath(x1, y1 - height / 2);
        path.lineTo(x2, y2 - height / 2);
        path.lineTo(x2, y2 + height / 2);
        path.lineTo(x1, y1 + height / 2);
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
    }
    repaint();
}

void MainPanel::setVelocitiesOfSelectedNotes(float vel) {
    for (Note &note : notes) {
        if (note.isSelected) {
            note.velocity = vel;
        }
    }
    editor->updateNotes(notes);
    repaint();
}
} // namespace audio_plugin