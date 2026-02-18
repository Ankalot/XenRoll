#include "XenRoll/TopPanel.h"
#include "XenRoll/PluginEditor.h"

namespace audio_plugin {
TopPanel::TopPanel(float bar_width_px, const int topPanel_height_px,
                   AudioPluginAudioProcessorEditor *editor, Parameters *params)
    : bar_width_px(bar_width_px), topPanel_height_px(topPanel_height_px), editor(editor),
      params(params), init_bar_width_px(bar_width_px) {
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
    if (event.mods.isLeftButtonDown()) {
        params->zones.turnZoneOn(time);
        editor->zonesChanged();
        repaint();
        return;
    }

    // Turn off zone
    if (event.mods.isRightButtonDown()) {
        params->zones.turnZoneOff(time);
        editor->zonesChanged();
        repaint();
        return;
    }
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
    g.setColour(Theme::bright);
    g.fillRect(clip);

    // Zones
    auto zp = std::vector<float>(params->zones.getZonesPoints().begin(),
                                 params->zones.getZonesPoints().end());
    zp.insert(zp.begin(), 0.0f);
    zp.push_back(float(params->get_num_bars()));
    auto zpOnOff = params->zones.getZonesOnOff();
    g.setColour(Theme::dark);
    for (int i = 0; i < zpOnOff.size(); ++i) {
        if (!zpOnOff[i]) {
            float leftPoint_px = juce::jmax(float(clipX), zp[i] * bar_width_px);
            float rightPoint_px =
                juce::jmin(float(clipX + clipWidth), zp[i + 1] * bar_width_px);
            if ((leftPoint_px < clipX + clipWidth) && (rightPoint_px > clipX)) {
                g.fillRect(leftPoint_px, 0.0f, rightPoint_px - leftPoint_px, float(topPanel_height_px));
            }
        }
    }

    // Zones points
    g.setColour(Theme::darkest);
    juce::Path trianglePath;
    const float zpw = adaptSize(zonePoint_collision_width_px);
    for (int i = 1; i < zp.size() - 1; ++i) {
        float zp_px = zp[i] * bar_width_px;
        if ((zp_px >= clipX) && (zp_px <= clipX + clipWidth)) {
            trianglePath.startNewSubPath(zp_px,
                                         topPanel_height_px - zonePoint_collision_width_px);
            trianglePath.lineTo(zp_px - zpw / 2, float(topPanel_height_px));
            trianglePath.lineTo(zp_px + zpw / 2, float(topPanel_height_px));
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
    g.setColour(Theme::darkest);
    int bar_i_start = static_cast<int>(clipX / bar_width_px);
    int bar_i_end =
        std::min(static_cast<int>((clipX + clipWidth) / bar_width_px) + 1, params->get_num_bars());
    for (int i = bar_i_start; i <= bar_i_end; ++i) {
        float xPos = i * bar_width_px;
        g.drawLine(xPos, 0.0f, xPos, float(topPanel_height_px), adaptSize(Theme::wide));
    }

    // Bar number
    g.setFont(adaptFont(Theme::big));
    for (int i = bar_i_start; i < bar_i_end; ++i) {
        int xPos = juce::roundToInt(i * bar_width_px);
        g.drawText(juce::String(i + 1),
                   juce::Rectangle<int>(xPos + static_cast<int>(adaptSize(10.0f)), 0,
                                        static_cast<int>(bar_width_px - adaptSize(10.0f)),
                                        topPanel_height_px),
                   juce::Justification::centredLeft, false);
    }

    // PlayHead
    if ((playHeadTime * bar_width_px >= clipX - playHeadWidth / 2) &&
        (playHeadTime * bar_width_px <= clipX + clipWidth + playHeadWidth / 2)) {

        juce::Path playHeadPath;
        playHeadPath.startNewSubPath(playHeadTime * bar_width_px, float(topPanel_height_px));
        playHeadPath.lineTo(playHeadTime * bar_width_px - playHeadWidth / 2,
                            float(topPanel_height_px - playHeadHeight));
        playHeadPath.lineTo(playHeadTime * bar_width_px + playHeadWidth / 2,
                            float(topPanel_height_px - playHeadHeight));
        playHeadPath.lineTo(playHeadTime * bar_width_px, float(topPanel_height_px));
        playHeadPath.closeSubPath();
        // g.setColour(Theme::brighter.withAlpha(1.0f));
        g.setColour(Theme::activated.withAlpha(1.0f));
        g.fillPath(playHeadPath);

        g.setColour(Theme::darkest);
        juce::PathStrokeType stroke(Theme::narrower);
        stroke.setJointStyle(juce::PathStrokeType::mitered);
        g.strokePath(playHeadPath, stroke);
    }
}
} // namespace audio_plugin