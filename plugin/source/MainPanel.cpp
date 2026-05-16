// clang-format off
#include "XenRoll/PluginEditor.h" // must be first!
#include "XenRoll/MainPanel.h"    // must be second!
// clang-format on
#include <random>

namespace audio_plugin {
MainPanel::MainPanel(AudioPluginAudioProcessorEditor *editor, Parameters *params)
    : editor(editor), params(params) {
    octave_height_px = params->octave_height_px;
    bar_width_px = params->bar_width_px;
    init_octave_height_px = params->init_octave_height_px;
    init_bar_width_px = params->init_bar_width_px;

    this->setSize(juce::roundToInt(params->get_num_bars() * bar_width_px),
                  juce::roundToInt(params->num_octaves * octave_height_px));
    setInterceptsMouseClicks(true, true);

    addKeyListener(this);
    setWantsKeyboardFocus(true);

    notes = editor->getNotes();
    unselectAllNotes();
    remakeKeys();

    bendFont = juce::Font(getLookAndFeel().getTypefaceForFont(NULL)).withPointHeight(Theme::small_);
}

MainPanel::~MainPanel() { removeKeyListener(this); }

void MainPanel::initViewport() {
    viewport = findParentComponentOfClass<juce::Viewport>();
    viewport->setViewPosition(params->lastViewPos);
}

const juce::PathStrokeType MainPanel::outlineStroke(Theme::wide, juce::PathStrokeType::mitered);

int MainPanel::totalCentsToY(int totalCents) {
    return juce::roundToInt(octave_height_px * (params->num_octaves - totalCents / 1200.0f));
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

    g.setColour(params->theme.darkest);
    g.strokePath(textPath, outlineStroke);

    g.setColour(params->theme.brightest);
    g.fillPath(textPath);
}

float MainPanel::adaptHor(float inputThickness) {
    return inputThickness * std::min(1.0f, octave_height_px / init_octave_height_px);
}

float MainPanel::adaptVert(float inputThickness) {
    return inputThickness * std::min(1.0f, bar_width_px / init_bar_width_px);
}

float MainPanel::adaptFont(float inputThickness) {
    return inputThickness * std::min(std::min(1.0f, (octave_height_px + init_octave_height_px) *
                                                        (0.5f) / init_octave_height_px),
                                     std::min(1.0f, (bar_width_px + init_bar_width_px) * (0.5f) /
                                                        init_bar_width_px));
}

void MainPanel::paint(juce::Graphics &g) {
    // We paint only visible area

    if (viewport == nullptr)
        return;
    juce::Rectangle<int> clip = viewport->getViewArea();
    int clipWidth = clip.getWidth();
    int clipHeight = clip.getHeight();
    int clipX = clip.getX();
    int clipY = clip.getY();
    g.setColour(params->theme.dark);
    g.fillRect(clip.toFloat());

    g.setColour(params->theme.darkest);
    // octaves
    int octave_i_start = static_cast<int>(clipY / octave_height_px);
    int octave_i_end = std::min(static_cast<int>((clipY + clipHeight) / octave_height_px) + 1,
                                params->num_octaves);
    const float adaptedHorWide = adaptHor(Theme::wide);
    for (int i = octave_i_start; i <= octave_i_end; ++i) {
        float yPos = i * octave_height_px;
        g.drawLine(float(clipX), yPos, float(clipX + clipWidth), yPos, adaptedHorWide);
    }
    // bars
    int bar_i_start = static_cast<int>(clipX / bar_width_px);
    int bar_i_end =
        std::min(static_cast<int>((clipX + clipWidth) / bar_width_px) + 1, params->get_num_bars());
    const float adaptedVertWide = adaptVert(Theme::wide);
    for (int i = bar_i_start; i <= bar_i_end; ++i) {
        float xPos = i * bar_width_px;
        g.drawLine(xPos, 0.0f, xPos, octave_height_px * params->num_octaves, adaptedVertWide);
    }
    // beats and subdivisions
    const float adaptedVertNarrow = adaptVert(Theme::narrow);
    const float adaptedVertNarrower = adaptVert(Theme::narrower);
    for (int i = bar_i_start; i < bar_i_end; ++i) {
        for (int j = 0; j < params->num_beats; ++j) {
            float xPos = (i + float(j) / params->num_beats) * bar_width_px;
            g.drawLine(xPos, 0.0f, xPos, octave_height_px * params->num_octaves, adaptedVertNarrow);
            for (int k = 1; k < params->num_subdivs; ++k) {
                float xPosSub =
                    xPos + float(k) / (params->num_subdivs * params->num_beats) * bar_width_px;
                g.drawLine(xPosSub, 0.0f, xPosSub, octave_height_px * params->num_octaves,
                           adaptedVertNarrower);
            }
        }
    }
    // keys
    const float adaptedHorNarrow = adaptHor(Theme::narrow);
    for (const int &key : keys) {
        for (int j = octave_i_start; j < octave_i_end; ++j) {
            float yPos = (j + 1.0f - float(key) / 1200) * octave_height_px;
            auto line = juce::Line<float>(0.0f, yPos, params->get_num_bars() * bar_width_px, yPos);
            if (params->generateNewKeys && keyIsGenNew[key]) {
                g.drawDashedLine(line, dashLengths, numDashLengths, adaptedHorNarrow);
            } else {
                g.drawLine(line, adaptedHorNarrow);
            }
        }
    }

    // PlayHead line
    if ((playHeadTime * bar_width_px >= clipX) &&
        (playHeadTime * bar_width_px <= clipX + clipWidth)) {
        // g.setColour(params->theme.brighter);
        g.setColour(params->theme.activated);
        g.drawLine(playHeadTime * bar_width_px, float(clipY), playHeadTime * bar_width_px,
                   float(clipY + clipHeight), Theme::narrower);
    }

    auto clipFloat = clip.toFloat();
    juce::PathStrokeType strokeType(Theme::narrower);
    strokeType.setJointStyle(juce::PathStrokeType::mitered);

    // ghost notes
    for (const Note &note : ghostNotes) {
        juce::Path notePath = getNotePath(note);
        // is needed only when params->showPitchesMemoryTraces
        if (notePath.getBounds().intersects(clipFloat)) {
            g.setColour(params->theme.darkest.interpolatedWith(params->theme.darker, 0.5));
            g.fillPath(notePath);
        }
    }

    // Audition line
    if (isAuditing) {
        g.setColour(params->theme.activated.darker(0.3f));
        for (const Note &note : notes) {
            if ((note.time <= auditionTime) && (auditionTime < note.time + note.duration)) {
                g.setColour(params->theme.activated);
                break;
            }
        }
        g.drawLine(auditionTime * bar_width_px, float(clipY), auditionTime * bar_width_px,
                   float(clipY + clipHeight), Theme::narrower);
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
                g.setColour(params->theme.dark.interpolatedWith(
                    params->theme.brighter.brighter(1.0f), note.velocity));
            }
            g.fillPath(notePath);
            if (note.isSelected) {
                g.setColour(params->theme.brightest);
            } else {
                g.setColour(params->theme.darkest);
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
                        "%c%d", (note.bend > 0 ? '+' : '-'), abs(note.bend));
                    drawOutlinedText(g, bendText, notePath.getBounds(), bendFont);
                }
            }
        }
    }

    // vocal notes
    if (params->vocalToMelody) {
        // already recorded vocal notes (but the recording hasn't ended yet)
        g.setColour(params->theme.activated);
        for (const Note &note : vocalNotes) {
            juce::Path notePath = getNotePath(note);
            if (notePath.getBounds().intersects(clipFloat)) {
                g.fillPath(notePath);
                g.strokePath(notePath, strokeType);
            }
        }
        // currently recording vocal note
        if (showRecNote) {
            juce::Path notePath = getNotePath(recNote);
            if (notePath.getBounds().intersects(clipFloat)) {
                g.fillPath(notePath);
                g.strokePath(notePath, strokeType);
            }
        }
    }

    // recorded manually played notes
    if (params->recordManuallyPlayedNotes) {
        g.setColour(params->theme.activated);
        for (const Note &note : editor->getRecordedManuallyPlayedNotes()) {
            juce::Path notePath = getNotePath(note);
            if (notePath.getBounds().intersects(clipFloat)) {
                g.fillPath(notePath);
                g.strokePath(notePath, strokeType);
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
        const float adaptedHorWider = adaptHor(Theme::wider);
        for (const auto &pts : pitchTraces.second) {
            const float timeStart = timeStamps[i];
            float timeEnd = params->get_num_bars();
            if (i + 1 != numTimeStamps) {
                timeEnd = timeStamps[i + 1];
            }
            const int posXstart = juce::roundToInt(timeStart * bar_width_px);
            const int posXend = juce::roundToInt(timeEnd * bar_width_px);
            if ((posXstart <= clipX + clipWidth) && (posXend >= clipX)) {
                for (int j = 0; j < numPitches; ++j) {
                    const float traceValue = pts.second[j];
                    const int posY = totalCentsToY(pitchTraces.first[j]);
                    if ((posY >= clipY) && (posY <= clipY + clipHeight)) {
                        const juce::uint8 brightness = juce::roundToInt(255 * traceValue);
                        g.setColour(juce::Colour::fromRGB(brightness, brightness, brightness));
                        g.drawLine(posXstart, posY, posXend, posY, adaptedHorWider);
                    }
                }
            }
            i++;
        }
    }

    // selecting rectangle
    if (isSelecting) {
        juce::Rectangle<int> selectionRect = juce::Rectangle(selectStartPoint, selectLastPoint);
        g.setColour(params->theme.darkest.withAlpha(0.5f));
        g.fillRect(selectionRect);

        g.setColour(params->theme.brightest);
        g.drawRect(selectionRect, int(Theme::narrow));
    }

    // pitch curve
    // this is 100%: (pitchCurve.first.size() == pitchCurve.second.size())
    const int pitchCurveSize = pitchCurve.first.size();
    if (pitchCurveSize != 0) {
        juce::Path path;
        bool curveBreak = true;
        const float adaptedHorWider = adaptHor(Theme::wider);
        const int clipXleft = clipX - juce::roundToInt(clipWidth * 0.05f);
        const int clipXright = clipX + juce::roundToInt(clipWidth * 1.05f);
        for (int i = 0; i < pitchCurveSize; ++i) {
            const int totalCents = pitchCurve.second[i];
            const float pointX = pitchCurve.first[i] * bar_width_px;
            if ((pointX >= clipXleft) && (pointX <= clipXright) && (totalCents != -1)) {
                const float pointY = totalCentsToY(totalCents);
                if (curveBreak) {
                    bool nextIsGap = (i == pitchCurveSize - 1) || (pitchCurve.second[i + 1] == -1);
                    if (nextIsGap) {
                        // Draw single point
                        // Draw outline
                        g.setColour(params->theme.darkest);
                        g.fillEllipse(pointX - (adaptedHorWider + adaptedHorWide) / 2,
                                      pointY - (adaptedHorWider + adaptedHorWide) / 2,
                                      adaptedHorWider + adaptedHorWide,
                                      adaptedHorWider + adaptedHorWide);
                        // Draw main
                        g.setColour(params->theme.activated);
                        g.fillEllipse(pointX - adaptedHorWider / 2, pointY - adaptedHorWider / 2,
                                      adaptedHorWider, adaptedHorWider);
                    } else {
                        // Start new segment
                        path.startNewSubPath(juce::Point<float>(pointX, pointY));
                    }
                } else {
                    // Add line to existing segment
                    path.lineTo(juce::Point<float>(pointX, pointY));
                }
                curveBreak = false;
            } else {
                curveBreak = true;
            }
        }

        // Draw the complete path
        if (!path.isEmpty()) {
            // Draw outline
            g.setColour(params->theme.darkest);
            g.strokePath(path, juce::PathStrokeType(adaptedHorWider + adaptedHorWide));
            // Draw main line
            g.setColour(params->theme.activated);
            g.strokePath(path, juce::PathStrokeType(adaptedHorWider));
        }
    }

    // ratios marks
    g.setColour(params->theme.activated);
    const auto fontSizeRatio = adaptFont(Theme::big);
    const auto fontSizeError = adaptFont(Theme::small_);
    const float adaptedGapX = adaptVert(5);
    const float adaptedGapY = adaptHor(3);
    for (const auto ratioMark : params->ratiosMarks) {
        float ratioMarkXPos = ratioMark.time * bar_width_px;

        float lowerKeyY = (params->num_octaves - float(ratioMark.getLowerKeyTotalCents()) / 1200) *
                          octave_height_px;
        float higherKeyY =
            (params->num_octaves - float(ratioMark.getHigherKeyTotalCents()) / 1200) *
            octave_height_px;

        int lowerNoteInd = ratioMark.getLowerNoteIndex();
        if (lowerNoteInd != -1) {
            float lowerNoteLeftX = notes[lowerNoteInd].time * bar_width_px;
            float lowerNoteRightX =
                (notes[lowerNoteInd].time + notes[lowerNoteInd].duration) * bar_width_px;
            if (ratioMarkXPos < lowerNoteLeftX) {
                if ((lowerNoteLeftX >= clipX) && (ratioMarkXPos <= clipX + clipWidth)) {
                    // outboard line for the lower note
                    g.drawLine(
                        juce::Line<float>(ratioMarkXPos, lowerKeyY, lowerNoteLeftX, lowerKeyY),
                        adaptedHorNarrow);
                }
            } else if (ratioMarkXPos > lowerNoteRightX) {
                if ((lowerNoteRightX <= clipX + clipWidth) && (ratioMarkXPos >= clipX)) {
                    // outboard line for the lower note
                    g.drawLine(
                        juce::Line<float>(ratioMarkXPos, lowerKeyY, lowerNoteRightX, lowerKeyY),
                        adaptedHorNarrow);
                }
            }
        }

        int higherNoteInd = ratioMark.getHigherNoteIndex();
        if (higherNoteInd != -1) {
            float higherNoteLeftX = notes[higherNoteInd].time * bar_width_px;
            float higherNoteRightX =
                (notes[higherNoteInd].time + notes[higherNoteInd].duration) * bar_width_px;
            if (ratioMarkXPos < higherNoteLeftX) {
                if ((higherNoteLeftX >= clipX) && (ratioMarkXPos <= clipX + clipWidth)) {
                    // outboard line for the higher note
                    g.drawLine(
                        juce::Line<float>(ratioMarkXPos, higherKeyY, higherNoteLeftX, higherKeyY),
                        adaptedHorNarrow);
                }
            } else if (ratioMarkXPos > higherNoteRightX) {
                if ((higherNoteRightX <= clipX + clipWidth) && (ratioMarkXPos >= clipX)) {
                    // outboard line for the higher note
                    g.drawLine(
                        juce::Line<float>(ratioMarkXPos, higherKeyY, higherNoteRightX, higherKeyY),
                        adaptedHorNarrow);
                }
            }
        }

        if ((ratioMarkXPos >= clipX) && (ratioMarkXPos <= clipX + clipWidth)) {
            // double arrow
            const auto point1 = juce::Point<float>(ratioMarkXPos, lowerKeyY);
            const float midY = (lowerKeyY + higherKeyY) / 2.0f;
            const auto point2 = juce::Point<float>(ratioMarkXPos, (lowerKeyY + higherKeyY) / 2.0f);
            const auto point3 = juce::Point<float>(ratioMarkXPos, higherKeyY);
            g.drawArrow(juce::Line<float>(point2, point1), adaptedVertWide, adaptedVertWide * 3,
                        adaptedVertWide * 3);
            g.drawArrow(juce::Line<float>(point2, point3), adaptedVertWide, adaptedVertWide * 3,
                        adaptedVertWide * 3);

            // ratio
            int num, den;
            std::tie(num, den) = ratioMark.getRatio();
            juce::String ratioMarkText = juce::String(num) + "/" + juce::String(den);
            g.setFont(fontSizeRatio);
            g.drawText(ratioMarkText,
                       juce::Rectangle<float>(ratioMarkXPos + adaptedGapX, midY - fontSizeRatio,
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
                           juce::Rectangle<float>(ratioMarkXPos + adaptedGapX, midY + adaptedGapY,
                                                  100, fontSizeError),
                           juce::Justification::centredLeft);
            }
        }
    }

    // ratio mark preline
    if (isDrawingRatioMark) {
        g.setColour(params->theme.activated);
        const auto point1 = ratioMarkStartPoint.toFloat();
        const auto point3 = ratioMarkLastPoint.toFloat();
        const auto point2 = (point1 + point3) / 2.0f;
        g.drawArrow(juce::Line<float>(point2, point1), adaptedVertWide, adaptedVertWide * 3,
                    adaptedVertWide * 3);
        g.drawArrow(juce::Line<float>(point2, point3), adaptedVertWide, adaptedVertWide * 3,
                    adaptedVertWide * 3);
    }
}

void MainPanel::unselectAllNotes() {
    for (int i = 0; i < notes.size(); ++i) {
        notes[i].isSelected = false;
    }
}

void MainPanel::quantizeSelectedNotes() {
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

    saveState();
    editor->updateNotes(notes);
    remakeKeys();
    repaint();
}

void MainPanel::randomizeSelectedNotesTiming() {
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

    saveState();
    editor->updateNotes(notes);
    remakeKeys();
    repaint();
}

void MainPanel::randomizeSelectedNotesVelocity() {
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

    saveState();
    editor->updateNotes(notes);
    repaint();
}

void MainPanel::deleteAllRatiosMarks() {
    params->ratiosMarks.clear();
    repaint();
    saveState();
}

void MainPanel::numBarsChanged() {
    bool deletedSmth = false;
    size_t notesNum = notes.size();
    for (int i = 0; i < notesNum; ++i) {
        if (notes[i].time + notes[i].duration > params->get_num_bars()) {
            deleteNote(i);
            i--;
            notesNum--;
            deletedSmth = true;
        }
    }
    deletedSmth = (std::erase_if(params->ratiosMarks,
                                 [&](const auto &ratioMark) {
                                     return (ratioMark.time > params->get_num_bars());
                                 }) > 0) ||
                  deletedSmth;
    if (viewport != nullptr) {
        float min_bar_width_px =
            static_cast<float>(viewport->getViewWidth()) / params->get_num_bars();
        if (bar_width_px < min_bar_width_px) {
            bar_width_px = min_bar_width_px;
            editor->changeBarWidthPx(bar_width_px);
        }
    }
    remakeKeys();
    updateLayout();

    if (deletedSmth) {
        saveState();
    }
}

void MainPanel::mouseWheelMove(const juce::MouseEvent &event,
                               const juce::MouseWheelDetails &wheel) {
    if (event.mods.isAltDown()) {
        wasBending = true;
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
        octave_height_px = octave_height_px * stretchFactor;
        float min_octave_height_px = static_cast<float>(viewHeight) / params->num_octaves;
        octave_height_px =
            juce::jlimit(min_octave_height_px, params->max_octave_height_px, octave_height_px);
        editor->changeOctaveHeightPx(octave_height_px);
    } else {
        bar_width_px = bar_width_px * stretchFactor;
        float min_bar_width_px = static_cast<float>(viewWidth) / params->get_num_bars();
        bar_width_px = juce::jlimit(min_bar_width_px, params->max_bar_width_px, bar_width_px);
        editor->changeBarWidthPx(bar_width_px);
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
    int octave = params->num_octaves - 1 - static_cast<int>(point.getY() / octave_height_px);
    int cents = static_cast<int>(round(
                    (1.0f - (fmodf(point.getY(), octave_height_px) / octave_height_px)) * 1200)) %
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

bool MainPanel::lineIntersectsRatioMark(const RatioMark &ratioMark, const juce::Line<int> &line) {
    float ratioMarkXPos = ratioMark.time * bar_width_px;
    float ratioMarkHighYPos =
        (params->num_octaves - float(ratioMark.getHigherKeyTotalCents()) / 1200) * octave_height_px;
    float ratioMarkLowYPos =
        (params->num_octaves - float(ratioMark.getLowerKeyTotalCents()) / 1200) * octave_height_px;
    float ratioMarkHeight = ratioMarkLowYPos - ratioMarkHighYPos;
    if (ratioMarkHeight < ratioMarkMinHeight) {
        ratioMarkLowYPos += (ratioMarkMinHeight - ratioMarkHeight) / 2.0f;
        ratioMarkHighYPos -= (ratioMarkMinHeight - ratioMarkHeight) / 2.0f;
    }

    auto ratioMarkRectangle =
        juce::Rectangle<float>(ratioMarkXPos - ratioMarkHalfWidth, ratioMarkHighYPos,
                               2 * ratioMarkHalfWidth, ratioMarkLowYPos - ratioMarkHighYPos);

    return ratioMarkRectangle.intersects(line.toFloat());
}

// Isn't called if already dragging
void MainPanel::mouseDown(const juce::MouseEvent &event) {
    grabKeyboardFocus();
    juce::Point<int> point = event.getPosition();
    lastDragPoint = point; // mouseDown isn't called if mouseDrag, so we can do this

    if (event.mods.isMiddleButtonDown()) {
        if (!params->editRatiosMarks) {
            for (int i = int(notes.size()) - 1; i >= 0; --i) {
                juce::Path notePath = getNotePath(notes[i]);
                if (notePath.contains(point.toFloat())) {
                    // Show velocity panel
                    notes[i].isSelected = true;
                    editor->showVelocityPanel(notes[i].velocity);
                    repaint();
                    return;
                }
            }
        }

        // Is panning (dragging view)
        isPanning = true;
        lastPanPos = getParentComponent()->getLocalPoint(this, lastDragPoint);
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }

    if (event.mods.isLeftButtonDown()) {
        if (params->editRatiosMarks) {
            for (auto &ratioMark : params->ratiosMarks) {
                if (pointOnRatioMark(ratioMark, point)) {
                    // Is moving ratio mark
                    isMovingRatioMark = true;
                    // Save initial ratio mark time
                    initialRatioMarkTimesForDrag.clear();
                    initialRatioMarkTimesForDrag.push_back(ratioMark.time);
                    // Save time pivot
                    dragPivotTime = point.getX() / bar_width_px;
                    // Save ref to ratio mark
                    movingRatioMark = &ratioMark;
                    return;
                }
            }

            // Is drawing ratio mark
            isDrawingRatioMark = true;
            ratioMarkStartPoint = point;
            ratioMarkLastPoint = point;
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

                    if (event.mods.isAltDown()) {
                        // Time-stretching selected notes
                        isTimeStretching = true;
                        // Set note's index
                        resizeClickNoteInd = i;
                        // Save initial state of selected notes (time and duration) & borders
                        initialStateForDrag.clear();
                        timeStretchSelectionLeft = std::numeric_limits<float>::max();
                        timeStretchSelectionRight = 0.0f;
                        for (const Note &note : notes) {
                            if (note.isSelected) {
                                initialStateForDrag.push_back({note.time, note.duration});
                                timeStretchSelectionLeft =
                                    std::min(timeStretchSelectionLeft, note.time);
                                timeStretchSelectionRight =
                                    std::max(timeStretchSelectionRight, note.time + note.duration);
                            }
                        }
                        // Save time pivot
                        dragPivotTime = point.getX() / bar_width_px;
                        // Save initial ratio mark times (all for simplicity)
                        initialRatioMarkTimesForDrag.clear();
                        for (const auto &rm : params->ratiosMarks) {
                            initialRatioMarkTimesForDrag.push_back(rm.time);
                        }
                        return;
                    }

                    // Is resizing selected notes
                    isResizing = true;
                    // Set note's index
                    resizeClickNoteInd = i;
                    // Save time pivot
                    dragPivotTime = point.getX() / bar_width_px;
                    // Save initial state of selected notes: time and duration
                    initialStateForDrag.clear();
                    for (const Note &note : notes) {
                        if (note.isSelected) {
                            initialStateForDrag.push_back({note.time, note.duration});
                        }
                    }
                    return;
                }

                if (notes[i].isSelected) {
                    if (!event.mods.isShiftDown()) {
                        needToUnselectAllNotesExcept = i;
                        needToUnselectAllNotesExcept_Ctrl = event.mods.isCtrlDown();
                    } else if (event.mods.isCtrlDown()) {
                        int cents = notes[i].cents;
                        for (Note &note : notes) {
                            if ((note.cents == cents) && params->zones.isNoteInActiveZone(note)) {
                                note.isSelected = true;
                            }
                        }
                    }
                } else {
                    if (!event.mods.isShiftDown()) {
                        unselectAllNotes();
                    }
                    if (event.mods.isCtrlDown()) {
                        int cents = notes[i].cents;
                        for (Note &note : notes) {
                            if ((note.cents == cents) && params->zones.isNoteInActiveZone(note)) {
                                note.isSelected = true;
                            }
                        }
                    }
                    notes[i].isSelected = true;
                    repaint();
                }

                if (GlobalSettings::getInstance().getPlayDraggedNotes()) {
                    std::lock_guard<std::mutex> lock(mptcMtx);
                    for (const Note &note : notes) {
                        if (note.isSelected) {
                            dragManuallyPlayedKeys.insert(
                                {note.cents + note.octave * 1200, note.velocity});
                        }
                    }
                    editor->setManuallyPlayedKeys(dragManuallyPlayedKeys, "drag");
                }

                // Is moving selected notes
                isMoving = true;
                // Save time pivot
                dragPivotTime = point.getX() / bar_width_px;
                // Save pitch pivot
                dragPivotPitch = params->num_octaves - point.getY() / octave_height_px;
                // Save initial state of selected notes: time, duration and totalCents
                initialStateForDrag.clear();
                initialTotalCentsForDrag.clear();
                for (const Note &note : notes) {
                    if (note.isSelected) {
                        initialStateForDrag.push_back({note.time, note.duration});
                        initialTotalCentsForDrag.push_back(note.octave * 1200 + note.cents);
                    }
                }
                return;
            }
        }

        // Unselect notes
        bool thereAreSelNotes = thereAreSelectedNotes();
        unselectAllNotes();
        if (thereAreSelNotes) {
            repaint();
            editor->hideVelocityPanel();
            return;
        }

        // Create note
        float time = point.getX() / bar_width_px;
        if (params->timeSnap) {
            time = timeToSnappedTime(time);
        }
        float duration = std::min(params->lastDuration, params->get_num_bars() - time);
        if (params->timeSnap) {
            duration = std::max(timeToSnappedTime(duration),
                                1.0f / (params->num_beats * params->num_subdivs));
        }
        params->lastDuration = duration;
        int octave, cents;
        std::tie(octave, cents) = pointToOctaveCents(point);
        if (params->keySnap && !keys.empty()) {
            std::tie(octave, cents) = centsToKeysCents(octave, cents);
        }
        notes.push_back({octave, cents, time, false, duration, params->lastVelocity});
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
        saveState();
        editor->updateNotes(notes);
        repaint();

        if (GlobalSettings::getInstance().getPlayDraggedNotes()) {
            // play placed note
            const int totalCents = cents + octave * 1200;
            {
                std::lock_guard<std::mutex> lock(mptcMtx);
                // we need to do this for playing a key that is already been played
                if (dragManuallyPlayedKeys.erase(totalCents) != 0) {
                    editor->setManuallyPlayedKeys(dragManuallyPlayedKeys, "drag");
                }
                dragManuallyPlayedKeys.insert({totalCents, params->lastVelocity});
                editor->setManuallyPlayedKeys(dragManuallyPlayedKeys, "drag");
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
                            safeThis->dragManuallyPlayedKeys.erase(totalCents);
                            safeThis->editor->setManuallyPlayedKeys(
                                safeThis->dragManuallyPlayedKeys, "drag");
                        }
                    }
                });
        }
    }

    if (event.mods.isRightButtonDown()) {
        if (params->editRatiosMarks) {
            // Delete ratio marks under cursor
            if (std::erase_if(params->ratiosMarks, [&](const auto &ratioMark) {
                    return pointOnRatioMark(ratioMark, point);
                }) > 0) {
                repaint();
                saveState();
            }
            return;
        }

        if (event.mods.isAltDown()) {
            // Is auditing notes under cursor
            isAuditing = true;
            auditionTime = point.getX() / bar_width_px;
            editor->startAuditing(auditionTime);
            repaint();
            setMouseCursor(juce::MouseCursor::CrosshairCursor);
            return;
        }

        // Is selecting
        isSelecting = true;
        // Start rectangular selection
        selectStartPoint = point;
        selectLastPoint = point;
        juce::Point<float> pointFloat = point.toFloat();
        // we start iterating with the newest notes
        int numNotes = (int)notes.size();
        for (int i = numNotes - 1; i >= 0; --i) {
            juce::Path notePath = getNotePath(notes[i]);
            if (notePath.contains(pointFloat)) {
                deleteNote(i);
                saveState();
                editor->updateNotes(notes);
                repaint();
                return;
            }
        }
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
    auto currDragPoint = event.getPosition();
    auto delta = currDragPoint - lastDragPoint;

    // Set time for clock diagram
    if (params->showClockDiagram && !editor->isPlaying()) {
        editor->setTimeClockDiagramPanel(currDragPoint.toFloat().getX() / bar_width_px);
    }

    // Auto-scroll when cursor is outside visible area (not when panning)
    if (viewport != nullptr && !isPanning) {
        const int scrollSpeed = 10;
        auto viewPos = viewport->getViewPosition();
        const int viewWidth = viewport->getViewWidth();
        const int viewHeight = viewport->getViewHeight();
        bool scrolled = false;

        // Horizontal scroll
        if (!params->isCamFixedOnPlayHead) {
            if (currDragPoint.getX() <= viewPos.getX()) {
                viewport->setViewPosition(viewPos.withX(viewPos.x - scrollSpeed));
                scrolled = true;
            } else if (currDragPoint.getX() >= viewPos.getX() + viewWidth) {
                viewport->setViewPosition(viewPos.withX(viewPos.x + scrollSpeed));
                scrolled = true;
            }
        }

        // Vertical scroll
        if (currDragPoint.getY() <= viewPos.getY()) {
            viewport->setViewPosition(viewPos.withY(viewPos.y - scrollSpeed));
            scrolled = true;
        } else if (currDragPoint.getY() >= viewPos.getY() + viewHeight) {
            viewport->setViewPosition(viewPos.withY(viewPos.y + scrollSpeed));
            scrolled = true;
        }

        if (scrolled) {
            params->lastViewPos = viewport->getViewPosition();
            editor->repaintTopPanel();
        }
    }

    // Panning (dragging view)
    if (isPanning) {
        auto currentPos = getParentComponent()->getLocalPoint(this, currDragPoint);
        auto panDelta = currentPos - lastPanPos;

        if (params->isCamFixedOnPlayHead) {
            panDelta.setX(0);
        }

        if (viewport != nullptr) {
            auto newPos = viewport->getViewPosition() - panDelta;
            params->lastViewPos = newPos;
            viewport->setViewPosition(newPos);
        }

        if (panDelta.getX() != 0) {
            editor->repaintTopPanel();
        }

        lastPanPos = currentPos;
    }

    // Resizing selected notes
    if (isResizing && (delta.getX() != 0)) {
        float cursorTime = currDragPoint.getX() / bar_width_px;
        float dtime = cursorTime - dragPivotTime;
        float dt = 1.0f / (params->num_beats * params->num_subdivs);

        // If timeSnap: dtime should be divisble by dt
        if (params->timeSnap) {
            dtime = juce::roundToInt(dtime / dt) * dt;
        }

        // Resizing shouldn't make any selected note's duration < minNoteDuration and
        //      it shouldn't make some selected note (time + duration) > num bars
        size_t idx = 0;
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected) {
                float time = initialStateForDrag[idx].first;
                float initialDuration = initialStateForDrag[idx].second;
                if (initialDuration + dtime < params->minNoteDuration) {
                    if (params->timeSnap) {
                        dtime = -std::floor((initialDuration - params->minNoteDuration) / dt) * dt;
                    } else {
                        dtime = params->minNoteDuration - initialDuration;
                    }
                }
                dtime = juce::jmin(dtime, params->get_num_bars() - initialDuration - time);
                idx++;
            }
        }

        if (prevDtime != dtime) {
            // Apply resizing to selected notes
            idx = 0;
            for (int i = 0; i < notes.size(); ++i) {
                if (notes[i].isSelected) {
                    float initialDuration = initialStateForDrag[idx].second;
                    notes[i].duration = initialDuration + dtime;
                    idx++;
                }
            }

            // Changed last duration
            if ((resizeClickNoteInd < notes.size()) && notes[resizeClickNoteInd].isSelected) {
                params->lastDuration = notes[resizeClickNoteInd].duration;
            }

            wasResizing = true;
            remakeKeys();
            editor->updateNotes(notes);
            repaint();
            prevDtime = dtime;
        }
    }

    // Time-stretching selected notes
    if (isTimeStretching && (delta.getX() != 0)) {
        float cursorTime = currDragPoint.getX() / bar_width_px;

        float pivotSpan = dragPivotTime - timeStretchSelectionLeft;
        if (std::abs(pivotSpan) < 1e-6f)
            return;
        float scale = (cursorTime - timeStretchSelectionLeft) / pivotSpan;

        // Time-stretching shouldn't make any selected note's duration < minNoteDuration
        size_t idx = 0;
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected) {
                float initialDuration = initialStateForDrag[idx].second;
                if (initialDuration * scale < params->minNoteDuration) {
                    scale = params->minNoteDuration / initialDuration;
                }
                idx++;
            }
        }
        // And it shouldn't make some selected note (time + duration) > num bars
        const float maxScale = (params->get_num_bars() - timeStretchSelectionLeft) /
                               (timeStretchSelectionRight - timeStretchSelectionLeft);
        scale = std::min(scale, maxScale);

        if (prevScale != scale) {
            // Apply time-stretching to selected notes
            idx = 0;
            for (int i = 0; i < notes.size(); ++i) {
                if (notes[i].isSelected) {
                    float initialTime = initialStateForDrag[idx].first;
                    float initialDuration = initialStateForDrag[idx].second;
                    notes[i].time =
                        timeStretchSelectionLeft + (initialTime - timeStretchSelectionLeft) * scale;
                    notes[i].duration = initialDuration * scale;
                    idx++;
                }
            }

            // Apply time-stretching to ratio marks that depend on selected notes
            for (size_t rmi = 0; rmi < params->ratiosMarks.size(); ++rmi) {
                auto &rm = params->ratiosMarks[rmi];
                const int lni = rm.getLowerNoteIndex();
                const int hni = rm.getHigherNoteIndex();
                if (((lni == -1) || notes[lni].isSelected) &&
                    ((hni == -1) || notes[hni].isSelected) && ((lni != -1) || (hni != -1))) {
                    float initialRMTime = initialRatioMarkTimesForDrag[rmi];
                    rm.time = timeStretchSelectionLeft +
                              (initialRMTime - timeStretchSelectionLeft) * scale;
                    rm.time = juce::jlimit(0.0f, (float)params->get_num_bars(), rm.time);
                }
            }

            // Changed last duration
            if ((resizeClickNoteInd < notes.size()) && notes[resizeClickNoteInd].isSelected) {
                params->lastDuration = notes[resizeClickNoteInd].duration;
            }

            wasTimeStretching = true;
            remakeKeys();
            editor->updateNotes(notes);
            repaint();
            prevScale = scale;
        }
    }

    // Moving ratio mark
    if (isMovingRatioMark && movingRatioMark && (delta.getX() != 0)) {
        float cursorTime = currDragPoint.getX() / bar_width_px;
        float dtime = cursorTime - dragPivotTime;

        // Time should be in range [0, num bars]
        float time = juce::jlimit(0.0f, (float)params->get_num_bars(),
                                  initialRatioMarkTimesForDrag[0] + dtime);

        // If timeSnap: time should be divisble by dt
        if (params->timeSnap) {
            float dt = 1.0f / (params->num_beats * params->num_subdivs);
            time = juce::roundToInt(time / dt) * dt;
        }

        if (prevTime != time) {
            // Apply movement to ratio mark
            movingRatioMark->time = time;

            wasMovingRatioMark = true;
            repaint();
            prevTime = time;
        }
    }

    // Moving selected notes
    if (isMoving) {
        needToUnselectAllNotesExcept = -1;
        float cursorTime = currDragPoint.getX() / bar_width_px;
        float cursorPitch = params->num_octaves - currDragPoint.getY() / octave_height_px;
        float dtime = cursorTime - dragPivotTime;
        int dcents = juce::roundToInt(1200 * (cursorPitch - dragPivotPitch));
        if (event.mods.isShiftDown()) {
            // Move vertically (pitch) slowly
            dcents = juce::roundToInt(dcents * vertMoveSlowCoef);
        }

        // If timeSnap: time should be divisble by dt (but this can change in next loop)
        if (params->timeSnap) {
            float dt = 1.0f / (params->num_beats * params->num_subdivs);
            dtime = juce::roundToInt(dtime / dt) * dt;
        }

        // Horizontal movement shouldn't make any selected note's time < 0 or
        //      (time + duration) > num bars
        // Vertical movement shouldn't make any selected note's totalCents < 0 or
        //      totalCents > num_octaves*1200 - 1
        size_t idx = 0;
        for (int i = 0; i < notes.size(); ++i) {
            if (notes[i].isSelected) {
                float initialTime = initialStateForDrag[idx].first;
                float duration = initialStateForDrag[idx].second;
                int initialTotalCents = initialTotalCentsForDrag[idx];
                dtime = juce::jmax(dtime, -initialTime);
                dtime = juce::jmin(dtime, params->get_num_bars() - initialTime - duration);
                dcents = juce::jmax(dcents, -initialTotalCents);
                dcents = juce::jmin(dcents, params->num_octaves * 1200 - 1 - initialTotalCents);
                idx++;
            }
        }

        bool changedTime = false;
        if (dtime != prevDtime) {
            // Apply horizontal movement
            idx = 0;
            for (int i = 0; i < notes.size(); ++i) {
                if (notes[i].isSelected) {
                    float newTime = initialStateForDrag[idx].first + dtime;
                    ;
                    notes[i].time = newTime;
                    idx++;
                }
            }

            changedTime = true;
        }

        bool changedPitch = false;
        if (dcents != prevDcents) {
            // Apply vertical movement
            if (params->keySnap) {
                if (initialTotalCentsForDrag.size() == 1) {
                    int initialTotalCents = initialTotalCentsForDrag[0];
                    int newTotalCents = initialTotalCents + dcents;
                    int newOctave = newTotalCents / 1200;
                    int newCents = newTotalCents - newOctave * 1200;
                    std::tie(newCents, newOctave) = findNearestKey(newCents, newOctave);
                    newTotalCents = newOctave * 1200 + newCents;
                    dcents = newTotalCents - initialTotalCents;
                    if ((dcents != prevDcents) && (newOctave < params->num_octaves) &&
                        (newOctave >= 0)) {
                        auto it = std::find_if(notes.begin(), notes.end(),
                                               [](const auto &note) { return note.isSelected; });
                        it->octave = newOctave;
                        it->cents = newCents;

                        changedPitch = true;
                    }
                }
            } else {
                idx = 0;
                for (int i = 0; i < notes.size(); ++i) {
                    if (notes[i].isSelected) {
                        int newTotalCents = initialTotalCentsForDrag[idx] + dcents;
                        int newOctave = newTotalCents / 1200;
                        int newCents = newTotalCents - newOctave * 1200;
                        notes[i].octave = newOctave;
                        notes[i].cents = newCents;
                        idx++;
                    }
                }

                changedPitch = true;
            }
        }

        bool updatedManuallyPlayedKeys = false;
        if (changedPitch && GlobalSettings::getInstance().getPlayDraggedNotes()) {
            // Play dragged vertically notes
            std::lock_guard<std::mutex> lock(mptcMtx);
            idx = 0;
            for (int i = 0; i < notes.size(); ++i) {
                if (notes[i].isSelected) {
                    dragManuallyPlayedKeys.erase(initialTotalCentsForDrag[idx] + prevDcents);
                    dragManuallyPlayedKeys.insert(
                        {notes[i].cents + notes[i].octave * 1200, notes[i].velocity});
                    idx++;
                }
            }

            updatedManuallyPlayedKeys = true;
        }

        if (changedTime) {
            timeCorrectRatioMarksBasedOnSelNotes(dtime - prevDtime);
        }
        if (changedPitch) {
            pitchCorrectRatioMarksBasedOnSelNotes();
        }
        if (changedTime || changedPitch) {
            wasMoving = true;
            remakeKeys(dcents - prevDcents);
            editor->updateNotes(notes);
            repaint();
            prevDtime = dtime;
            prevDcents = dcents;
        }
        if (updatedManuallyPlayedKeys) {
            std::lock_guard<std::mutex> lock(mptcMtx);
            editor->setManuallyPlayedKeys(dragManuallyPlayedKeys, "drag");
        }
    }

    // Is auditing notes under cursor
    if (isAuditing) {
        float newAuditionTime = currDragPoint.getX() / bar_width_px;
        if (auditionTime != newAuditionTime) {
            auditionTime = newAuditionTime;
            editor->setAuditionTime(auditionTime);
            repaint();
        }
    }

    // Is selecting
    if (isSelecting) {
        selectLastPoint = currDragPoint;
        repaint();
    }

    // Is drawing ratio mark
    if (isDrawingRatioMark) {
        ratioMarkLastPoint = currDragPoint;
        auto currentMods = juce::ModifierKeys::getCurrentModifiers();
        if (currentMods.isRightButtonDown()) {
            // Stop drawing ratio mark
            isDrawingRatioMark = false;
        }
        repaint();
    }

    // Is deleting ratio marks under cursor
    if (params->editRatiosMarks && event.mods.isRightButtonDown()) {
        if (std::erase_if(params->ratiosMarks, [&](const auto &ratioMark) {
                return lineIntersectsRatioMark(ratioMark,
                                               juce::Line<int>(lastDragPoint, currDragPoint));
            }) > 0) {
            repaint();
            saveState();
        }
    }

    lastDragPoint = currDragPoint;
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
    if (isMoving && GlobalSettings::getInstance().getPlayDraggedNotes()) {
        std::lock_guard<std::mutex> lock(mptcMtx);
        for (const Note &note : notes) {
            if (note.isSelected) {
                dragManuallyPlayedKeys.erase(note.cents + note.octave * 1200);
            }
        }
        editor->setManuallyPlayedKeys(dragManuallyPlayedKeys, "drag");
    }

    if (wasResizing || wasMoving || wasTimeStretching || wasMovingRatioMark) {
        saveState();
    }

    isPanning = false;
    wasResizing = false;
    isResizing = false;
    isMoving = false;
    wasMoving = false;
    isTimeStretching = false;
    wasTimeStretching = false;
    isMovingRatioMark = false;
    wasMovingRatioMark = false;
    prevScale = 1.0f;
    prevDtime = 0.0f;
    prevDcents = 0;
    prevTime = -1.0f;
    setMouseCursor(juce::MouseCursor::NormalCursor);

    if (needToUnselectAllNotesExcept != -1) {
        unselectAllNotes();
        if (needToUnselectAllNotesExcept_Ctrl) {
            int cents = notes[needToUnselectAllNotesExcept].cents;
            for (Note &note : notes) {
                if ((note.cents == cents) && params->zones.isNoteInActiveZone(note)) {
                    note.isSelected = true;
                }
            }
        }
        notes[needToUnselectAllNotesExcept].isSelected = true;
        needToUnselectAllNotesExcept = -1;
    }

    if (isAuditing) {
        editor->endAuditing();
        isAuditing = false;
    }

    if (isSelecting) {
        if (!event.mods.isShiftDown()) {
            unselectAllNotes();
        }
        juce::Rectangle<int> selectionRect = juce::Rectangle(selectStartPoint, selectLastPoint);
        for (int i = 0; i < notes.size(); ++i) {
            juce::Path notePath = getNotePath(notes[i]);
            if (doesPathIntersectRect(notePath, selectionRect.toFloat())) {
                notes[i].isSelected = true;
            }
        }
        isSelecting = false;
    }

    if (isDrawingRatioMark) {
        if ((ratioMarkStartPoint != ratioMarkLastPoint) && !keys.empty()) {
            int startKeyOctave, startKeyCents, lastKeyOctave, lastKeyCents;

            std::tie(startKeyOctave, startKeyCents) = pointToOctaveCents(ratioMarkStartPoint);
            std::tie(startKeyCents, startKeyOctave) = findNearestKey(startKeyCents, startKeyOctave);
            int startKeyTotalCents = startKeyOctave * 1200 + startKeyCents;

            std::tie(lastKeyOctave, lastKeyCents) = pointToOctaveCents(ratioMarkLastPoint);
            std::tie(lastKeyCents, lastKeyOctave) = findNearestKey(lastKeyCents, lastKeyOctave);
            int lastKeyTotalCents = lastKeyOctave * 1200 + lastKeyCents;

            if (startKeyTotalCents != lastKeyTotalCents) {
                float startTime = ratioMarkStartPoint.getX() / bar_width_px;
                float lastTime = ratioMarkLastPoint.getX() / bar_width_px;

                // let start key be lower than last key
                if (startKeyTotalCents > lastKeyTotalCents) {
                    int x = startKeyTotalCents;
                    startKeyTotalCents = lastKeyTotalCents;
                    lastKeyTotalCents = x;
                    float y = startTime;
                    startTime = lastTime;
                    lastTime = y;
                }

                float time = ratioMarkLastPoint.getX() / bar_width_px;
                if (params->timeSnap) {
                    time = timeToSnappedTime(time);
                    startTime = timeToSnappedTime(startTime);
                    lastTime = timeToSnappedTime(lastTime);
                }

                int startNoteIndex = -1, lastNoteIndex = -1;
                const int numNotes = notes.size();
                for (int i = 0; i < numNotes; ++i) {
                    if ((startNoteIndex == -1) &&
                        ((notes[i].octave * 1200 + notes[i].cents) == startKeyTotalCents) &&
                        (notes[i].time <= startTime) &&
                        (startTime <= notes[i].time + notes[i].duration)) {
                        startNoteIndex = i;
                    } else if ((lastNoteIndex == -1) &&
                               ((notes[i].octave * 1200 + notes[i].cents) == lastKeyTotalCents) &&
                               (notes[i].time <= lastTime) &&
                               (lastTime <= notes[i].time + notes[i].duration)) {
                        lastNoteIndex = i;
                    }
                }

                RatioMark ratioMark(startKeyTotalCents, lastKeyTotalCents, time, params,
                                    startNoteIndex, lastNoteIndex);
                params->ratiosMarks.push_back(ratioMark);
                saveState();
            }
        }
        isDrawingRatioMark = false;
    }

    repaint();
}

// Isn't called if already dragging
void MainPanel::mouseMove(const juce::MouseEvent &event) {
    if (params->showClockDiagram && !editor->isPlaying()) {
        editor->setTimeClockDiagramPanel(event.getPosition().toFloat().getX() / bar_width_px);
    }

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
    for (const auto &note : notes) {
        if ((note.cents == note_cents) && (params->zones.isNoteInActiveZone(note)))
            num_notes_cents_i++;
    }

    int num_notes_cents_i_ghost_not_visible = 0;
    for (const auto &note : ghostNotes) {
        if (note.cents == note_cents) {
            if (params->showGhostNotesKeys && params->zones.isNoteInActiveZone(note)) {
                num_notes_cents_i++;
            } else {
                num_notes_cents_i_ghost_not_visible++;
            }
        }
    }

    for (RatioMark &ratioMark : params->ratiosMarks) {
        if (ratioMark.getLowerNoteIndex() > i) {
            ratioMark.setLowerNoteIndex(ratioMark.getLowerNoteIndex() - 1);
        } else if (ratioMark.getLowerNoteIndex() == i) {
            ratioMark.setLowerNoteIndex(-1);
        }

        if (ratioMark.getHigherNoteIndex() > i) {
            ratioMark.setHigherNoteIndex(ratioMark.getHigherNoteIndex() - 1);
        } else if (ratioMark.getHigherNoteIndex() == i) {
            ratioMark.setHigherNoteIndex(-1);
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

void MainPanel::reattachRatiosMarks(int dcents) {
    if (!params->autoCorrectRatiosMarks) {
        return;
    }

    if (dcents != 0) {
        // Reattach by moving up/down by dcents
        auto it = params->ratiosMarks.begin();
        while (it != params->ratiosMarks.end()) {
            RatioMark &ratioMark = *it;
            bool changed = false;
            int higherKeyTotalCents = ratioMark.getHigherKeyTotalCents();
            int higherKeyCents = higherKeyTotalCents % 1200;
            if (!keysFromAllNotes.contains(higherKeyCents)) {
                higherKeyTotalCents += dcents;
                changed = true;
            }
            int lowerKeyTotalCents = ratioMark.getLowerKeyTotalCents();
            int lowerKeyCents = lowerKeyTotalCents % 1200;
            if (!keysFromAllNotes.contains(lowerKeyCents)) {
                lowerKeyTotalCents = lowerKeyTotalCents + dcents;
                changed = true;
            }
            if (changed) {
                ratioMark.setKeysTotalCents(lowerKeyTotalCents, higherKeyTotalCents);
            }
            // Delete ratio mark if it is 1/1 and doesn't depend on notes
            if ((higherKeyTotalCents == lowerKeyTotalCents) &&
                (ratioMark.getLowerNoteIndex() == -1) && (ratioMark.getHigherNoteIndex() == -1)) {
                it = params->ratiosMarks.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        // Reattach by finding nearest key
        const int maxCentsChange = 100;
        auto it = params->ratiosMarks.begin();
        while (it != params->ratiosMarks.end()) {
            RatioMark &ratioMark = *it;
            bool changed = false;
            int higherKeyTotalCents = ratioMark.getHigherKeyTotalCents();
            int higherKeyCents = higherKeyTotalCents % 1200;
            if (!keysFromAllNotes.contains(higherKeyCents)) {
                auto result =
                    findNearestKeyWithLimit(higherKeyCents, maxCentsChange, keysFromAllNotes);
                if (result) {
                    int higherKeyOctave = higherKeyTotalCents / 1200;
                    int newHigherKeyCents = *result;
                    if ((newHigherKeyCents - higherKeyCents < -maxCentsChange) &&
                        (higherKeyOctave < params->num_octaves - 1)) {
                        higherKeyTotalCents = 1200 * (higherKeyOctave + 1) + newHigherKeyCents;
                    } else if ((newHigherKeyCents - higherKeyCents > maxCentsChange) &&
                               (higherKeyOctave > 0)) {
                        higherKeyTotalCents = 1200 * (higherKeyOctave - 1) + newHigherKeyCents;
                    } else {
                        higherKeyTotalCents = 1200 * higherKeyOctave + newHigherKeyCents;
                    }
                    changed = true;
                }
            }
            int lowerKeyTotalCents = ratioMark.getLowerKeyTotalCents();
            int lowerKeyCents = lowerKeyTotalCents % 1200;
            if (!keysFromAllNotes.contains(lowerKeyCents)) {
                auto result =
                    findNearestKeyWithLimit(lowerKeyCents, maxCentsChange, keysFromAllNotes);
                if (result) {
                    int lowerKeyOctave = lowerKeyTotalCents / 1200;
                    int newLowerKeyCents = *result;
                    if ((newLowerKeyCents - lowerKeyCents < -maxCentsChange) &&
                        (lowerKeyOctave < params->num_octaves - 1)) {
                        lowerKeyTotalCents = 1200 * (lowerKeyOctave + 1) + newLowerKeyCents;
                    } else if ((newLowerKeyCents - lowerKeyCents > maxCentsChange) &&
                               (lowerKeyOctave > 0)) {
                        lowerKeyTotalCents = 1200 * (lowerKeyOctave - 1) + newLowerKeyCents;
                    } else {
                        lowerKeyTotalCents = 1200 * lowerKeyOctave + newLowerKeyCents;
                    }
                    changed = true;
                }
            }
            if (changed) {
                ratioMark.setKeysTotalCents(lowerKeyTotalCents, higherKeyTotalCents);
            }
            // Delete ratio mark if it is 1/1 and doesn't depend on notes
            if ((higherKeyTotalCents == lowerKeyTotalCents) &&
                (ratioMark.getLowerNoteIndex() == -1) && (ratioMark.getHigherNoteIndex() == -1)) {
                it = params->ratiosMarks.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void MainPanel::remakeKeys(int dcents) {
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
    reattachRatiosMarks(dcents);
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

void MainPanel::restoreState() {
    const State &restoredState = params->stateHistory.getCurrent();
    notes = restoredState.notes;
    params->ratiosMarks = restoredState.ratiosMarks;
    int restoredNumBars = restoredState.numBars;
    if (restoredNumBars != params->get_num_bars()) {
        editor->changeNumBars(restoredNumBars);
    }
    unselectAllNotes();
    editor->hideVelocityPanel();
    remakeKeys();
    editor->updateNotes(notes);
    repaint();
}

void MainPanel::pitchCorrectRatioMarksBasedOnSelNotes() {
    for (RatioMark &ratioMark : params->ratiosMarks) {
        bool changed = false;
        const int lni = ratioMark.getLowerNoteIndex();
        int lowerKeyTotalCents = ratioMark.getLowerKeyTotalCents();
        if ((lni != -1) && notes[lni].isSelected) {
            lowerKeyTotalCents = notes[lni].octave * 1200 + notes[lni].cents;
            changed = true;
        }
        const int hni = ratioMark.getHigherNoteIndex();
        int higherKeyTotalCents = ratioMark.getHigherKeyTotalCents();
        if ((hni != -1) && notes[hni].isSelected) {
            higherKeyTotalCents = notes[hni].octave * 1200 + notes[hni].cents;
            changed = true;
        }
        if (changed) {
            ratioMark.setKeysTotalCents(lowerKeyTotalCents, higherKeyTotalCents);
        }
    }
}

void MainPanel::timeCorrectRatioMarksBasedOnSelNotes(float dtime) {
    for (RatioMark &ratioMark : params->ratiosMarks) {
        const int lni = ratioMark.getLowerNoteIndex();
        const int hni = ratioMark.getHigherNoteIndex();
        if (((lni == -1) || (notes[lni].isSelected)) && ((hni == -1) || (notes[hni].isSelected)) &&
            ((lni != -1) || (hni != -1))) {
            ratioMark.time =
                juce::jlimit(0.0f, (float)params->get_num_bars(), ratioMark.time + dtime);
        }
    }
}

bool MainPanel::keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) {
    juce::ignoreUnused(originatingComponent);

    // select all notes
    if (key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0)) {
        selectAllNotes();
        return true;
    }

    // unselect all notes
    if (key == juce::KeyPress::escapeKey) {
        unselectAllNotes();
        repaint();
        editor->hideVelocityPanel();
        return true;
    }

    // delete selected notes
    if (key == juce::KeyPress::deleteKey) {
        int numNotes = int(notes.size());
        for (int i = 0; i < numNotes; ++i) {
            if (notes[i].isSelected) {
                deleteNote(i);
                i--;
                numNotes--;
            }
        }
        saveState();
        editor->updateNotes(notes);
        repaint();
        return true;
    }

    // copy selected notes
    if (key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0)) {
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
    if (key == juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0)) {
        unselectAllNotes();
        editor->hideVelocityPanel();
        bool needGenNewKeys = false;
        bool needUpdateKeys = false;
        for (int i = 0; i < copiedNotes.size(); ++i) {
            notes.push_back(copiedNotes[i]);
            int cents = copiedNotes[i].cents;
            if (params->zones.isNoteInActiveZone(copiedNotes[i])) {
                auto [_, inserted] = keys.insert(cents);
                if (inserted || keyIsGenNew[cents]) {
                    keyIsGenNew[cents] = false;
                    if (params->generateNewKeys) {
                        needGenNewKeys = true;
                    }
                    needUpdateKeys = true;
                }
            }
            keysFromAllNotes.insert(cents);
        }
        if (needGenNewKeys) {
            generateNewKeys();
        }
        if (needUpdateKeys) {
            editor->updateKeys(keys);
        }
        editor->showMessage(juce::String(copiedNotes.size()) + " note" +
                            (copiedNotes.size() == 1 ? "" : "s") + " pasted!");
        saveState();
        editor->updateNotes(notes);
        repaint();
        return true;
    }

    // Undo
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)) {
        if (params->stateHistory.canUndo()) {
            params->stateHistory.undo();
            restoreState();
        }
        return true;
    }

    // Redo
    if (key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0)) {
        if (params->stateHistory.canRedo()) {
            params->stateHistory.redo();
            restoreState();
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
        int cents = getCentsFromMessage();
        if (cents != -1) {
            cents = cents % 1200;
            for (int i = 0; i < notes.size(); ++i) {
                if (notes[i].isSelected) {
                    notes[i].cents = cents;
                }
            }
            pitchCorrectRatioMarksBasedOnSelNotes();
            remakeKeys();
            saveState();
            editor->updateNotes(notes);
            repaint();
        }
        return true;
    }

    // increase the pitch of selected notes by cents
    if (key == juce::KeyPress(juce::KeyPress::returnKey, juce::ModifierKeys::shiftModifier, 0)) {
        int dcents = getCentsFromMessage();
        if (dcents != -1) {
            // dcents is limited: no selected note should have
            //                    totalCents >= params->num_octaves * 1200
            for (const Note &note : notes) {
                if (note.isSelected) {
                    int totalCents = note.octave * 1200 + note.cents;
                    dcents = juce::jmin(dcents, params->num_octaves * 1200 - 1 - totalCents);
                }
            }
            if (dcents != 0) {
                for (Note &note : notes) {
                    if (note.isSelected) {
                        int newTotalCents = note.octave * 1200 + note.cents + dcents;
                        note.octave = newTotalCents / 1200;
                        note.cents = newTotalCents - note.octave * 1200;
                    }
                }
                pitchCorrectRatioMarksBasedOnSelNotes();
                remakeKeys(dcents);
                saveState();
                editor->updateNotes(notes);
                repaint();
            }
        }
        return true;
    }

    // decrease the pitch of selected notes by cents
    if (key == juce::KeyPress(juce::KeyPress::returnKey, juce::ModifierKeys::ctrlModifier, 0)) {
        int dcents = getCentsFromMessage();
        if (dcents != -1) {
            dcents = -dcents; // NEGATIVE!
            // dcents is limited: no selected note should have
            //                    totalCents < 0
            for (const Note &note : notes) {
                if (note.isSelected) {
                    int totalCents = note.octave * 1200 + note.cents;
                    dcents = juce::jmax(dcents, -totalCents);
                }
            }
            if (dcents != 0) {
                for (Note &note : notes) {
                    if (note.isSelected) {
                        int newTotalCents = note.octave * 1200 + note.cents + dcents;
                        note.octave = newTotalCents / 1200;
                        note.cents = newTotalCents - note.octave * 1200;
                    }
                }
                pitchCorrectRatioMarksBasedOnSelNotes();
                remakeKeys(dcents);
                saveState();
                editor->updateNotes(notes);
                repaint();
            }
        }
        return true;
    }

    // moving notes horizontally
    if ((key == juce::KeyPress::rightKey) ||
        (key == juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::shiftModifier, 0))) {
        if (!thereAreSelectedNotes()) {
            return false;
        }
        float dtime = 1.0f / (params->num_beats * params->num_subdivs);
        if (key.getModifiers().isShiftDown()) {
            dtime = 1.0f;
        }
        // dtime is limited: no selected note should have
        //                   time + duration > num bars
        int numBars = params->get_num_bars();
        for (const Note &note : notes) {
            if (note.isSelected) {
                dtime = juce::jmin(dtime, numBars - note.time - note.duration);
            }
        }
        if (dtime > 1e-5f) {
            for (Note &note : notes) {
                if (note.isSelected) {
                    note.time += dtime;
                }
            }
            remakeKeys(); // because notes can enter/leave time active/disabled zones
            timeCorrectRatioMarksBasedOnSelNotes(dtime);
            saveState();
            editor->updateNotes(notes);
            repaint();
        }
        return true;
    }
    if ((key == juce::KeyPress::leftKey) ||
        (key == juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::shiftModifier, 0))) {
        if (!thereAreSelectedNotes()) {
            return false;
        }
        // NEGATIVE!
        float dtime = -1.0f / (params->num_beats * params->num_subdivs);
        if (key.getModifiers().isShiftDown()) {
            dtime = -1.0f;
        }
        // dtime is limited: no selected note should have
        //                   time < 0
        for (const Note &note : notes) {
            if (note.isSelected) {
                dtime = juce::jmax(dtime, -note.time);
            }
        }
        if (dtime < -1e-5f) {
            for (Note &note : notes) {
                if (note.isSelected) {
                    note.time += dtime;
                }
            }
            remakeKeys(); // because notes can enter/leave time active/disabled zones
            timeCorrectRatioMarksBasedOnSelNotes(dtime);
            saveState();
            editor->updateNotes(notes);
            repaint();
        }
        return true;
    }

    // raising or lowering selected notes by a cent (or by a key in key snap mode)
    if (key == juce::KeyPress::upKey) {
        if (!thereAreSelectedNotes()) {
            return false;
        }
        if (params->keySnap) {
            // if some selected note due to raise up by a key will have octave = num octaves
            //    then abort
            for (const Note &note : notes) {
                if ((note.isSelected) && (note.octave == params->num_octaves - 1) &&
                    (note.cents == *keys.rbegin())) {
                    return true;
                }
            }
            for (Note &note : notes) {
                if (note.isSelected) {
                    int keyInd = getKeyIndex(note.cents) + 1;
                    if (keyInd == keys.size()) {
                        note.octave++;
                        note.cents = *keys.begin();
                    } else {
                        note.cents = *(std::next(keys.begin(), keyInd));
                    }
                }
            }
            pitchCorrectRatioMarksBasedOnSelNotes();
            remakeKeys();
        } else {
            // if some selected note due to raise up by a cent will have octave = num octaves
            //    then abort
            for (const Note &note : notes) {
                if ((note.isSelected) && (note.octave == params->num_octaves - 1) &&
                    (note.cents == 1199)) {
                    return true;
                }
            }
            for (Note &note : notes) {
                if (note.isSelected) {
                    note.cents++;
                    if (note.cents == 1200) {
                        note.octave++;
                        note.cents = 0;
                    }
                }
            }
            pitchCorrectRatioMarksBasedOnSelNotes();
            remakeKeys(1);
        }
        saveState();
        editor->updateNotes(notes);
        repaint();
        return true;
    }
    if (key == juce::KeyPress::downKey) {
        if (!thereAreSelectedNotes()) {
            return false;
        }
        if (params->keySnap) {
            // if some selected note due to lower down by a key will have octave = -1
            //    then abort
            for (const Note &note : notes) {
                if ((note.isSelected) && (note.octave == 0) && (note.cents == *keys.begin())) {
                    return true;
                }
            }
            for (Note &note : notes) {
                if (note.isSelected) {
                    int keyInd = getKeyIndex(note.cents) - 1;
                    if (keyInd == -1) {
                        note.octave--;
                        note.cents = *keys.rbegin();
                    } else {
                        note.cents = *(std::next(keys.begin(), keyInd));
                    }
                }
            }
            pitchCorrectRatioMarksBasedOnSelNotes();
            remakeKeys();
        } else {
            // if some selected note due to lower down by a cent will have octave = -1
            //    then abort
            for (const Note &note : notes) {
                if ((note.isSelected) && (note.octave == 0) && (note.cents == 0)) {
                    return true;
                }
            }
            for (Note &note : notes) {
                if (note.isSelected) {
                    note.cents--;
                    if (note.cents == -1) {
                        note.octave--;
                        note.cents = 1199;
                    }
                }
            }
            pitchCorrectRatioMarksBasedOnSelNotes();
            remakeKeys(-1);
        }
        saveState();
        editor->updateNotes(notes);
        repaint();
        return true;
    }

    // raising or lowering selected notes by an octave
    if (key == juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::shiftModifier, 0)) {
        // if some selected note due to raise up by an octave will have octave = num octaves
        //    then abort
        for (const Note &note : notes) {
            if (note.isSelected) {
                if (note.octave == params->num_octaves - 1) {
                    return true;
                }
            }
        }
        for (Note &note : notes) {
            if (note.isSelected) {
                note.octave++;
            }
        }
        pitchCorrectRatioMarksBasedOnSelNotes();
        saveState();
        editor->updateNotes(notes);
        repaint();
        return true;
    }
    if (key == juce::KeyPress(juce::KeyPress::downKey, juce::ModifierKeys::shiftModifier, 0)) {
        // if some selected note due to lower down by an octave will have octave = -1
        //    then abort
        for (const Note &note : notes) {
            if (note.isSelected) {
                if (note.octave == 0) {
                    return true;
                }
            }
        }
        for (Note &note : notes) {
            if (note.isSelected) {
                note.octave--;
            }
        }
        pitchCorrectRatioMarksBasedOnSelNotes();
        saveState();
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
            keyboardManuallyPlayedKeys.insert({totalCents, params->defaultVelocity});
            editor->setManuallyPlayedKeys(keyboardManuallyPlayedKeys, "keyboard");
            wasKeyDown[keyChar] = totalCents;
        }
        return true;
    }

    return false;
}

void MainPanel::modifierKeysChanged(const juce::ModifierKeys &modifiers) {
    if (wasBending && !modifiers.isAltDown()) {
        wasBending = false;
        saveState();
    }
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
                keyboardManuallyPlayedKeys.erase(totalCents);
                editor->setManuallyPlayedKeys(keyboardManuallyPlayedKeys, "keyboard");
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
            return ((den != 0) && (num >= den)) ? juce::roundToInt(1200 * log2(float(num) / den))
                                                : -1;
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
    int currWidth = getWidth();
    int currHeight = getHeight();
    int newWidth = juce::roundToInt(params->get_num_bars() * bar_width_px);
    int newHeight = juce::roundToInt(params->num_octaves * octave_height_px);

    // Adjust points on main panel
    float horScale = (float)newWidth / currWidth;
    float vertScale = (float)newHeight / currHeight;

    lastDragPoint.setX(juce::roundToInt(lastDragPoint.getX() * horScale));
    lastDragPoint.setY(juce::roundToInt(lastDragPoint.getY() * vertScale));

    selectStartPoint.setX(juce::roundToInt(selectStartPoint.getX() * horScale));
    selectStartPoint.setY(juce::roundToInt(selectStartPoint.getY() * vertScale));

    selectLastPoint.setX(juce::roundToInt(selectLastPoint.getX() * horScale));
    selectLastPoint.setY(juce::roundToInt(selectLastPoint.getY() * vertScale));

    ratioMarkStartPoint.setX(juce::roundToInt(ratioMarkStartPoint.getX() * horScale));
    ratioMarkStartPoint.setY(juce::roundToInt(ratioMarkStartPoint.getY() * vertScale));

    ratioMarkLastPoint.setX(juce::roundToInt(ratioMarkLastPoint.getX() * horScale));
    ratioMarkLastPoint.setY(juce::roundToInt(ratioMarkLastPoint.getY() * vertScale));

    // Set new size of main panel
    this->setSize(newWidth, newHeight);
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

juce::Path MainPanel::getNotePath(const Note &note) {
    float height;
    if (params->constNoteRectHeight) {
        height = init_octave_height_px * params->noteRectHeightCoef;
    } else {
        height = octave_height_px * params->noteRectHeightCoef;
    }
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
        if (y2 > y1) {
            path.startNewSubPath(x1, y1 - height / 2);
            path.lineTo(x2, y1 - height / 2);
            path.lineTo(x2, y2 + height / 2);
            path.lineTo(x1, y2 + height / 2);
            path.closeSubPath();
        } else {
            path.startNewSubPath(x1, y1 + height / 2);
            path.lineTo(x2, y1 + height / 2);
            path.lineTo(x2, y2 - height / 2);
            path.lineTo(x1, y2 - height / 2);
            path.closeSubPath();
        }
    } else {
        // make hexagon
        if (dy > 0) {
            float denom = dy - 2 * height;
            float z;
            if (abs(denom) < 1e-6) {
                z = dy * height / (2 * width);
            } else {
                z = height * (sqrt(width * width + dy * dy - 2 * height * dy) - width) / denom;
            }
            path.startNewSubPath(x1, y1 - height / 2);
            path.lineTo(x2 - z, y2 - height / 2);
            path.lineTo(x2, y2 - height / 2);
            path.lineTo(x2, y2 + height / 2);
            path.lineTo(x1 + z, y1 + height / 2);
            path.lineTo(x1, y1 + height / 2);
            path.closeSubPath();
        } else {
            dy = -dy;
            float denom = dy - 2 * height;
            float z;
            if (abs(denom) < 1e-6) {
                z = dy * height / (2 * width);
            } else {
                z = height * (sqrt(width * width + dy * dy - 2 * height * dy) - width) / denom;
            }
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

void MainPanel::setPlayHeadTime(float newPlayHeadTime) { playHeadTime = newPlayHeadTime; }

void MainPanel::updateNotes(const std::vector<Note> &new_notes) {
    for (RatioMark &ratioMark : params->ratiosMarks) {
        ratioMark.setLowerNoteIndex(-1);
        ratioMark.setHigherNoteIndex(-1);
    }
    notes = new_notes;
    remakeKeys();
    repaint();
}

void MainPanel::addRecordedNotes(const std::vector<Note> &recordedNotes) {
    unselectAllNotes();
    const int numBars = params->get_num_bars();
    int numAddedNotes = 0;
    for (auto note : recordedNotes) {
        if (note.time + note.duration <= numBars) {
            notes.push_back(note);
            numAddedNotes++;
        }
    }
    saveState();
    editor->updateNotes(notes);
    remakeKeys();
    editor->showMessage("Recorded " + juce::String(numAddedNotes) + " notes!");
}

void MainPanel::updateVocalNotes(const std::vector<Note> &newVocalNotes) {
    vocalNotes = newVocalNotes;
}

void MainPanel::hideRecNote() { showRecNote = false; }

void MainPanel::updateRecNote(const Note &newRecNote) {
    recNote = newRecNote;
    showRecNote = true;
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

void MainPanel::createNotesFromGhostNotes() {
    unselectAllNotes();
    int num_bars = params->get_num_bars();
    for (Note note : ghostNotes) {
        if (note.time + note.duration <= num_bars) {
            note.isSelected = true;
            notes.push_back(note);
        }
    }
    saveState();
    editor->updateNotes(notes);
    if (!params->showGhostNotesKeys) {
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
    params->lastVelocity = vel;
    editor->updateNotes(notes);
    repaint();
}
} // namespace audio_plugin