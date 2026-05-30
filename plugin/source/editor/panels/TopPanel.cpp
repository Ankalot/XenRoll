#include "XenRoll/editor/panels/TopPanel.h"
#include "XenRoll/editor/PluginEditor.h"

namespace audio_plugin {
TopPanel::TopPanel(const int topPanel_height_px, AudioPluginAudioProcessorEditor *editor,
                   Parameters *params)
    : topPanel_height_px(topPanel_height_px), editor(editor), params(params) {
    bar_width_px = params->bar_width_px;
    init_bar_width_px = params->init_bar_width_px;

    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    this->setSize(juce::roundToInt(params->get_num_bars() * bar_width_px), topPanel_height_px);
}

void TopPanel::mouseDown(const juce::MouseEvent &event) {
    editor->bringBackKeyboardFocus();
    juce::Point<int> point = event.getPosition();
    float time = point.getX() / bar_width_px;
    float precision = zonePoint_collision_width_px / bar_width_px / 2;

    // Add zone point
    if (event.mods.isLeftButtonDown() && event.mods.isShiftDown()) {
        float timePoint = time;
        if (params->timeSnap) {
            timePoint = 1.0f / (params->num_beats * params->num_subdivs) *
                        round(time / (1.0f / (params->num_beats * params->num_subdivs)));
        }
        bool pointAdded = params->zones.addPoint(timePoint);
        if (pointAdded) {
            // editor->zonesChanged();
            repaint();
        }
        return;
    }

    // Remove zone point
    if (event.mods.isRightButtonDown() && event.mods.isShiftDown()) {
        bool pointRemoved = params->zones.removePoint(time, precision);
        if (pointRemoved) {
            editor->zonesChanged();
            repaint();
        }
        return;
    }

    // Turn on zone
    if (event.mods.isLeftButtonDown() && event.mods.isAltDown()) {
        params->zones.turnZoneOn(time);
        editor->zonesChanged();
        repaint();
        return;
    }

    // Turn off zone
    if (event.mods.isRightButtonDown() && event.mods.isAltDown()) {
        params->zones.turnZoneOff(time);
        editor->zonesChanged();
        repaint();
        return;
    }

    // Set playhead time via OSC
    if (event.mods.isLeftButtonDown()) {
        float jumpTime = time;
        if (params->timeSnap) {
            float dt = 1.0f / (params->num_beats * params->num_subdivs);
            jumpTime = dt * juce::roundToInt(time / dt);
        }
        editor->sendOSCTransportPosition(jumpTime);
        prevDragPlayHeadTime = jumpTime;
        isMovingPlayHead = true;
        return;
    }
}

void TopPanel::mouseDrag(const juce::MouseEvent &event) {
    // Set playhead time via OSC
    if (isMovingPlayHead) {
        float time = event.position.getX() / bar_width_px;
        if (params->timeSnap) {
            float dt = 1.0f / (params->num_beats * params->num_subdivs);
            time = dt * juce::roundToInt(time / dt);
        }

        if (time != prevDragPlayHeadTime) {
            editor->sendOSCTransportPosition(time);
            prevDragPlayHeadTime = time;
        }
    }
}

void TopPanel::mouseUp(const juce::MouseEvent &event) {
    isMovingPlayHead = false;
    prevDragPlayHeadTime = -1.0f;
}

void TopPanel::paint(juce::Graphics &g) {
    // =======================================================================================
    //                                    PAINT ONLY VISIBLE AREA
    // =======================================================================================
    auto *viewport = findParentComponentOfClass<juce::Viewport>();
    if (viewport == nullptr)
        return;
    juce::Rectangle<int> clip = viewport->getViewArea();
    int clipWidth = clip.getWidth();
    int clipX = clip.getX();
    g.setColour(params->theme.bright);
    g.fillRect(clip);

    // Zones
    auto zp = std::vector<float>(params->zones.getZonesPoints().begin(),
                                 params->zones.getZonesPoints().end());
    zp.insert(zp.begin(), 0.0f);
    zp.push_back(static_cast<float>(params->get_num_bars()));
    auto zpOnOff = params->zones.getZonesOnOff();
    g.setColour(params->theme.dark);
    // Without this, I see a super thin light strip on my screen between two adjacent off zones.
    const float delta = 0.5f;
    for (int i = 0; i < zpOnOff.size(); ++i) {
        if (!zpOnOff[i]) {
            float leftPoint_px =
                juce::jmax(static_cast<float>(clipX), zp[i] * bar_width_px) - delta;
            float rightPoint_px =
                juce::jmin(static_cast<float>(clipX + clipWidth), zp[i + 1] * bar_width_px + delta);
            if ((leftPoint_px < clipX + clipWidth) && (rightPoint_px > clipX)) {
                g.fillRect(leftPoint_px, 0.0f, rightPoint_px - leftPoint_px,
                           static_cast<float>(topPanel_height_px));
            }
        }
    }

    // Zones points
    g.setColour(params->theme.darkest);
    juce::Path trianglePath;
    const float zpw = juce::jmax(3.0f, adaptSize(zonePoint_collision_width_px));
    for (int i = 1; i < zp.size() - 1; ++i) {
        float zp_px = zp[i] * bar_width_px;
        if ((zp_px >= clipX) && (zp_px <= clipX + clipWidth)) {
            trianglePath.startNewSubPath(zp_px, topPanel_height_px - zonePoint_collision_width_px);
            trianglePath.lineTo(zp_px - zpw / 2, static_cast<float>(topPanel_height_px));
            trianglePath.lineTo(zp_px + zpw / 2, static_cast<float>(topPanel_height_px));
            trianglePath.lineTo(zp_px, topPanel_height_px - zonePoint_collision_width_px);
            trianglePath.closeSubPath();
            g.fillPath(trianglePath);

            trianglePath.startNewSubPath(zp_px, zonePoint_collision_width_px);
            trianglePath.lineTo(zp_px - zpw / 2, 0.0f);
            trianglePath.lineTo(zp_px + zpw / 2, 0.0f);
            trianglePath.lineTo(zp_px, zonePoint_collision_width_px);
            trianglePath.closeSubPath();
            g.fillPath(trianglePath);

            // g.drawLine(zp_px, 0, zp_px, topPanel_height_px, Theme::wider);
        }
    }

    // Bars
    g.setColour(params->theme.darkest);
    int barStep = std::ceil(params->min_bar_width_px / bar_width_px);
    int bar_i_start = (static_cast<int>(clipX / bar_width_px) / barStep) * barStep;
    int rawEnd = std::ceil((clipX + clipWidth) / bar_width_px);
    int alignedEnd = ((rawEnd + barStep - 1) / barStep) * barStep;
    int bar_i_end = juce::jmin(alignedEnd, params->get_num_bars());
    const float barLineThickness = juce::jmax(1.0f, adaptSize(Theme::wide));
    for (int i = bar_i_start; i <= bar_i_end; i += barStep) {
        float xPos = i * bar_width_px;
        g.drawLine(xPos, 0.0f, xPos, static_cast<float>(topPanel_height_px), barLineThickness);
    }

    // Bar number
    g.setFont(adaptFont(Theme::big));
    const int barNumOffset = juce::roundToInt(adaptFont(6.0f));
    for (int i = bar_i_start; i < bar_i_end; i += barStep) {
        int xPos = juce::roundToInt(i * bar_width_px);
        g.drawText(juce::String(i + 1),
                   juce::Rectangle<int>(xPos + barNumOffset, 0,
                                        static_cast<int>(barStep * bar_width_px) - barNumOffset,
                                        topPanel_height_px),
                   juce::Justification::centredLeft, false);
    }

    // PlayHead
    if ((playHeadTime * bar_width_px >= clipX - playHeadWidth / 2) &&
        (playHeadTime * bar_width_px <= clipX + clipWidth + playHeadWidth / 2)) {

        juce::Path playHeadPath;
        playHeadPath.startNewSubPath(playHeadTime * bar_width_px,
                                     static_cast<float>(topPanel_height_px));
        playHeadPath.lineTo(playHeadTime * bar_width_px - playHeadWidth / 2,
                            static_cast<float>(topPanel_height_px - playHeadHeight));
        playHeadPath.lineTo(playHeadTime * bar_width_px + playHeadWidth / 2,
                            static_cast<float>(topPanel_height_px - playHeadHeight));
        playHeadPath.lineTo(playHeadTime * bar_width_px, static_cast<float>(topPanel_height_px));
        playHeadPath.closeSubPath();
        // g.setColour(params->theme.brighter.withAlpha(1.0f));
        g.setColour(params->theme.activated.withAlpha(1.0f));
        g.fillPath(playHeadPath);

        g.setColour(params->theme.darkest);
        juce::PathStrokeType stroke(Theme::narrower);
        stroke.setJointStyle(juce::PathStrokeType::mitered);
        g.strokePath(playHeadPath, stroke);
    }
}
} // namespace audio_plugin