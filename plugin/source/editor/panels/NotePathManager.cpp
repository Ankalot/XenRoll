#include "XenRoll/editor/panels/NotePathManager.h"

namespace audio_plugin {

void NotePathManager::addConditionalArc(juce::Path &path, float centreX, float centreY,
                                        float radius, float fromRadians, float toRadians,
                                        bool startAsNewSubPath) const {
    const float angleDelta = std::abs(toRadians - fromRadians);

    if (angleDelta >= juce::MathConstants<float>::halfPi) {
        path.addCentredArc(centreX, centreY, radius, radius, 0.0f, fromRadians, toRadians,
                           startAsNewSubPath);
        return;
    }

    const float sagitta = radius * (1.0f - std::cos(angleDelta * 0.5f));

    if (sagitta > MAX_VISUAL_DEVIATION) {
        path.addCentredArc(centreX, centreY, radius, radius, 0.0f, fromRadians, toRadians,
                           startAsNewSubPath);
    } else {
        float startX = centreX + radius * std::sin(fromRadians);
        float startY = centreY - radius * std::cos(fromRadians);

        float endX = centreX + radius * std::sin(toRadians);
        float endY = centreY - radius * std::cos(toRadians);

        if (startAsNewSubPath || path.isEmpty()) {
            path.startNewSubPath(startX, startY);
        } else {
            path.lineTo(startX, startY);
        }
        path.lineTo(endX, endY);
    }
}

juce::Path NotePathManager::buildCanonicalPath(float w, float dy, float h,
                                               float noteRoundCoef) const {
    constexpr float x1 = 0;
    const float x2 = x1 + w;
    constexpr float y1 = 0;
    const float y2 = y1 - dy;

    const float maxR = juce::jmin(h, w) * 0.5f;
    const float r = noteRoundCoef * juce::jmin(h, w) * 0.5f;

    constexpr float pi = juce::MathConstants<float>::pi;
    constexpr float halfPi = juce::MathConstants<float>::halfPi;
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
    const float halfW = w / 2;
    const float halfH = h / 2;

    juce::Path path;
    if (h >= w) {
        // make vertical rectangle
        if (dy == 0) {
            // no bend (y1 == y2)
            if ((maxR - r < TOL_RADIUS) && (r >= TOL_RADIUS)) {
                // full rounding (top & bottom)
                const juce::Point<float> v0 = {x1 + halfW, y1 - halfH + halfW}; // top circle
                const juce::Point<float> v1 = {x2 - halfW, y1 + halfH - halfW}; // bot circle

                path.addCentredArc(v0.getX(), v0.getY(), halfW, halfW, 0, -halfPi, halfPi, true);
                path.addCentredArc(v1.getX(), v1.getY(), halfW, halfW, 0, halfPi, pi + halfPi);
                path.closeSubPath();
            } else if (r < TOL_RADIUS) {
                // no rounding
                path.startNewSubPath(x1, y1 - halfH); // top-left corner
                path.lineTo(x2, y1 - halfH);          // top-right corner
                path.lineTo(x2, y1 + halfH);          // bot-right corner
                path.lineTo(x1, y1 + halfH);          // bot-left corner
                path.closeSubPath();
            } else {
                // 4 corners rounding
                const juce::Point<float> v0 = {x1 + r, y1 - halfH + r}; // top-left circle
                const juce::Point<float> v1 = {x2 - r, y1 - halfH + r}; // top-right circle
                const juce::Point<float> v2 = {x2 - r, y1 + halfH - r}; // bot-right circle
                const juce::Point<float> v3 = {x1 + r, y1 + halfH - r}; // bot-left circle

                path.addCentredArc(v0.getX(), v0.getY(), r, r, 0, -halfPi, 0, true);
                path.addCentredArc(v1.getX(), v1.getY(), r, r, 0, 0, halfPi);
                path.addCentredArc(v2.getX(), v2.getY(), r, r, 0, halfPi, pi);
                path.addCentredArc(v3.getX(), v3.getY(), r, r, 0, pi, pi + halfPi);
                path.closeSubPath();
            }
        } else if (dy < 0) {
            // negative bend
            if ((maxR - r < TOL_RADIUS) && (r >= TOL_RADIUS)) {
                // full rounding (top & bottom)
                const juce::Point<float> v0 = {x1 + halfW, y1 - halfH + halfW}; // top circle
                const juce::Point<float> v1 = {x2 - halfW, y2 + halfH - halfW}; // bot circle

                path.addCentredArc(v0.getX(), v0.getY(), halfW, halfW, 0, -halfPi, halfPi, true);
                path.addCentredArc(v1.getX(), v1.getY(), halfW, halfW, 0, halfPi, pi + halfPi);
                path.closeSubPath();
            } else if (r < TOL_RADIUS) {
                // edges rounding (top & bottom)
                const juce::Point<float> v0 = {x1, y1 + halfH}; // bot-left corner/circle
                const juce::Point<float> v1 = {x2, y2 - halfH}; // top-right corner/circle

                if ((y2 - halfH) < (y1 + halfH - std::sqrt(h * h - w * w))) {
                    const float g = v0.getDistanceFrom(v1); // dist between v0 and v1
                    const float alpha = std::acos(w / g) + std::acos(h / g);

                    addConditionalArc(path, v0.getX(), v0.getY(), h, 0, halfPi - alpha, true);
                    path.lineTo(v1);
                    addConditionalArc(path, v1.getX(), v1.getY(), h, pi, pi + halfPi - alpha);
                    path.lineTo(v0);
                    path.closeSubPath();
                } else {
                    const float alpha = std::acos(w / h);

                    addConditionalArc(path, v0.getX(), v0.getY(), h, 0, halfPi - alpha, true);
                    addConditionalArc(path, v1.getX(), v1.getY(), h, pi, pi + halfPi - alpha);
                    path.closeSubPath();
                }
            } else {
                // 4 corners rounding
                const juce::Point<float> v0 = {x1 + r, y1 - halfH + r}; // top-left circle
                const juce::Point<float> v1 = {x1 + r, y1 + halfH - r}; // bot-left circle
                const juce::Point<float> v2 = {x2 - r, y2 + halfH - r}; // bot-right circle
                const juce::Point<float> v3 = {x2 - r, y2 - halfH + r}; // top-right circle

                const float g = v1.getDistanceFrom(v3);
                if (((h - r) < (g + r)) && (v1.getY() > v3.getY())) {
                    // 4 corners & parts of top and bot edges
                    const float alpha = std::acos((h - 2 * r) / g) + std::acos((w - 2 * r) / g);

                    path.addCentredArc(v0.getX(), v0.getY(), r, r, 0, -halfPi, 0, true);
                    addConditionalArc(path, v1.getX(), v1.getY(), h - r, 0, halfPi - alpha);
                    addConditionalArc(path, v3.getX(), v3.getY(), r, halfPi - alpha, halfPi);
                    path.addCentredArc(v2.getX(), v2.getY(), r, r, 0, halfPi, pi);
                    addConditionalArc(path, v3.getX(), v3.getY(), h - r, pi, pi + halfPi - alpha);
                    addConditionalArc(path, v1.getX(), v1.getY(), r, pi + halfPi - alpha,
                                      pi + halfPi);
                    path.closeSubPath();
                } else {
                    // 2 corners & parts of top and bot edges
                    const float alpha = std::acos((w - r) / (h - r));

                    path.addCentredArc(v0.getX(), v0.getY(), r, r, 0, -halfPi, 0, true);
                    addConditionalArc(path, v1.getX(), v1.getY(), h - r, 0, halfPi - alpha);
                    path.addCentredArc(v2.getX(), v2.getY(), r, r, 0, halfPi, pi);
                    addConditionalArc(path, v3.getX(), v3.getY(), h - r, pi, pi + halfPi - alpha);
                    path.closeSubPath();
                }
            }
        } else {
            // positive bend
            if ((maxR - r < TOL_RADIUS) && (r >= TOL_RADIUS)) {
                // full rounding (top & bottom)
                const juce::Point<float> v0 = {x1 + halfW, y2 - halfH + halfW}; // top circle
                const juce::Point<float> v1 = {x2 - halfW, y1 + halfH - halfW}; // bot circle

                path.addCentredArc(v0.getX(), v0.getY(), halfW, halfW, 0, -halfPi, halfPi, true);
                path.addCentredArc(v1.getX(), v1.getY(), halfW, halfW, 0, halfPi, pi + halfPi);
                path.closeSubPath();
            } else if (r < TOL_RADIUS) {
                // edges rounding (top & bottom)
                const juce::Point<float> v0 = {x2, y2 + halfH}; // bot-right corner/circle
                const juce::Point<float> v1 = {x1, y1 - halfH}; // top=left corner/circle

                if ((y1 - halfH) < (y2 + h / 2 - std::sqrt(h * h - w * w))) {
                    const float g = v0.getDistanceFrom(v1);
                    const float alpha = std::acos(w / g) + std::acos(h / g);

                    addConditionalArc(path, v0.getX(), v0.getY(), h, 0, alpha - halfPi, true);
                    path.lineTo(v1);
                    addConditionalArc(path, v1.getX(), v1.getY(), h, pi, halfPi + alpha);
                    path.lineTo(v0);
                    path.closeSubPath();
                } else {
                    float alpha = std::acos(w / h);

                    addConditionalArc(path, v0.getX(), v0.getY(), h, 0, alpha - halfPi, true);
                    addConditionalArc(path, v1.getX(), v1.getY(), h, pi, halfPi + alpha);
                    path.closeSubPath();
                }
            } else {
                // 4 corners rounding
                const juce::Point<float> v0 = {x1 + r, y1 - halfH + r}; // top-left circle
                const juce::Point<float> v1 = {x1 + r, y1 + halfH - r}; // bot-left circle
                const juce::Point<float> v2 = {x2 - r, y2 + halfH - r}; // bot-right circle
                const juce::Point<float> v3 = {x2 - r, y2 - halfH + r}; // top-right circle

                const float g = v2.getDistanceFrom(v0);
                if (((h - r) < (g + r)) && (v2.getY() > v0.getY())) {
                    // 4 corners & parts of top and bot edges
                    const float alpha = std::acos((h - 2 * r) / g) + std::acos((w - 2 * r) / g);

                    path.addCentredArc(v3.getX(), v3.getY(), r, r, 0, halfPi, 0, true);
                    addConditionalArc(path, v2.getX(), v2.getY(), h - r, 0, alpha - halfPi);
                    addConditionalArc(path, v0.getX(), v0.getY(), r, alpha - halfPi, -halfPi);
                    path.addCentredArc(v1.getX(), v1.getY(), r, r, 0, -halfPi, -pi);
                    addConditionalArc(path, v0.getX(), v0.getY(), h - r, -pi, -pi - halfPi + alpha);
                    addConditionalArc(path, v2.getX(), v2.getY(), r, -pi - halfPi + alpha,
                                      -pi - halfPi);
                    path.closeSubPath();
                } else {
                    // 2 corners & parts of top and bot edges
                    const float alpha = std::acos((w - r) / (h - r));

                    path.addCentredArc(v3.getX(), v3.getY(), r, r, 0, halfPi, 0, true);
                    addConditionalArc(path, v2.getX(), v2.getY(), h - r, 0, alpha - halfPi);
                    path.addCentredArc(v1.getX(), v1.getY(), r, r, 0, -halfPi, -pi);
                    addConditionalArc(path, v0.getX(), v0.getY(), h - r, -pi, -pi - halfPi + alpha);
                    path.closeSubPath();
                }
            }
        }
    } else if (dy == 0) {
        // make horizontal rectangle
        // no bend (y1 == y2)
        if ((maxR - r < TOL_RADIUS) && (r >= TOL_RADIUS)) {
            // full rounding (left & right)
            path.addCentredArc(x1 + halfH, y1, halfH, halfH, 0, pi, twoPi, true); // left circle
            path.addCentredArc(x2 - halfH, y1, halfH, halfH, 0, 0, pi);           // right circle
            path.closeSubPath();
        } else if (r < TOL_RADIUS) {
            // no rounding
            path.startNewSubPath(x1, y1 - halfH); // top-left corner
            path.lineTo(x2, y1 - halfH);          // top-right corner
            path.lineTo(x2, y1 + halfH);          // bot-right corner
            path.lineTo(x1, y1 + halfH);          // bot-left corner
            path.closeSubPath();
        } else {
            // 4 corners rounding
            const juce::Point<float> v0 = {x1 + r, y1 - halfH + r}; // top-left circle
            const juce::Point<float> v1 = {x2 - r, y1 - halfH + r}; // top-right circle
            const juce::Point<float> v2 = {x2 - r, y1 + halfH - r}; // bot-right circle
            const juce::Point<float> v3 = {x1 + r, y1 + halfH - r}; // bot-left circle

            path.addCentredArc(v0.getX(), v0.getY(), r, r, 0, -halfPi, 0, true);
            path.addCentredArc(v1.getX(), v1.getY(), r, r, 0, 0, halfPi);
            path.addCentredArc(v2.getX(), v2.getY(), r, r, 0, halfPi, pi);
            path.addCentredArc(v3.getX(), v3.getY(), r, r, 0, pi, pi + halfPi);
            path.closeSubPath();
        }
    } else {
        // make bending shape
        if (dy < 0) {
            // negative bend
            dy = -dy;
            if ((maxR - r < TOL_RADIUS) && (r >= TOL_RADIUS)) {
                // full rounding (left & right)
                const juce::Point<float> v0 = {x1 + halfH, y1}; // left circle
                const juce::Point<float> v1 = {x2 - halfH, y2}; // right circle

                const float alpha = v0.getAngleToPoint(v1) - halfPi;

                path.addCentredArc(v0.getX(), v0.getY(), halfH, halfH, 0, pi + alpha, twoPi + alpha,
                                   true);
                path.addCentredArc(v1.getX(), v1.getY(), halfH, halfH, 0, alpha, pi + alpha);
                path.closeSubPath();
            } else if (r < TOL_RADIUS) {
                // no rounding (but parts of top & bot edges)
                const juce::Point<float> v0 = {x1, y1 + halfH}; // bot-left corner & circle
                const juce::Point<float> v1 = {x2, y2 - halfH}; // top-right corner & circle

                const float g = v0.getDistanceFrom(v1);
                const float alpha = v0.getAngleToPoint(v1) - std::acos(h / g);

                addConditionalArc(path, v0.getX(), v0.getY(), h, 0, alpha, true);
                path.lineTo(v1.getX(), v1.getY());
                addConditionalArc(path, v1.getX(), v1.getY(), h, pi, pi + alpha);
                path.lineTo(v0.getX(), v0.getY());
                path.closeSubPath();
            } else {
                // 4 corners rounding (and parts of top & bot edges)
                const juce::Point<float> v0 = {x1 + r, y1 - halfH + r}; // top-left circle
                const juce::Point<float> v1 = {x2 - r, y2 - halfH + r}; // top-right circle
                const juce::Point<float> v2 = {x2 - r, y2 + halfH - r}; // bot-right circle
                const juce::Point<float> v3 = {x1 + r, y1 + halfH - r}; // bot-left circle

                const float g = v3.getDistanceFrom(v1);
                const float alpha = v3.getAngleToPoint(v1) - std::acos((h - 2 * r) / g);

                path.addCentredArc(v0.getX(), v0.getY(), r, r, 0, -halfPi, 0, true);
                addConditionalArc(path, v3.getX(), v3.getY(), h - r, 0, alpha);
                addConditionalArc(path, v1.getX(), v1.getY(), r, alpha, halfPi);
                path.addCentredArc(v2.getX(), v2.getY(), r, r, 0, halfPi, pi);
                addConditionalArc(path, v1.getX(), v1.getY(), h - r, pi, pi + alpha);
                addConditionalArc(path, v3.getX(), v3.getY(), r, pi + alpha, pi + halfPi);
                path.closeSubPath();
            }
        } else {
            // positive bend
            if ((maxR - r < TOL_RADIUS) && (r >= TOL_RADIUS)) {
                // full rounding (left & right)
                const juce::Point<float> v0 = {x1 + halfH, y1}; // left circle
                const juce::Point<float> v1 = {x2 - halfH, y2}; // right circle

                const float alpha = v0.getAngleToPoint(v1) - halfPi;

                path.addCentredArc(v0.getX(), v0.getY(), halfH, halfH, 0, pi + alpha, twoPi + alpha,
                                   true);
                path.addCentredArc(v1.getX(), v1.getY(), halfH, halfH, 0, alpha, pi + alpha);
                path.closeSubPath();
            } else if (r < TOL_RADIUS) {
                // no rounding (but parts of top & bot edges)
                const juce::Point<float> v0 = {x2, y2 + halfH}; // bot-right corner & circle
                const juce::Point<float> v1 = {x1, y1 - halfH}; // top-left corner & circle

                const float g = v0.getDistanceFrom(v1);
                const float alpha = v0.getAngleToPoint(v1) + std::acos(h / g);

                addConditionalArc(path, v0.getX(), v0.getY(), h, 0, alpha, true);
                path.lineTo(v1.getX(), v1.getY());
                addConditionalArc(path, v1.getX(), v1.getY(), h, pi, pi + alpha);
                path.lineTo(v0.getX(), v0.getY());
                path.closeSubPath();
            } else {
                // 4 corners rounding (and parts of top & bot edges)
                const juce::Point<float> v0 = {x1 + r, y1 - halfH + r}; // top-left circle
                const juce::Point<float> v1 = {x2 - r, y2 - halfH + r}; // top-right circle
                const juce::Point<float> v2 = {x2 - r, y2 + halfH - r}; // bot-right circle
                const juce::Point<float> v3 = {x1 + r, y1 + halfH - r}; // bot-left circle

                const float g = v2.getDistanceFrom(v0);
                const float alpha = v2.getAngleToPoint(v0) + std::acos((h - 2 * r) / g);

                path.addCentredArc(v1.getX(), v1.getY(), r, r, 0, halfPi, 0, true);
                addConditionalArc(path, v2.getX(), v2.getY(), h - r, 0, alpha);
                addConditionalArc(path, v0.getX(), v0.getY(), r, alpha, -halfPi);
                path.addCentredArc(v3.getX(), v3.getY(), r, r, 0, -halfPi, -pi);
                addConditionalArc(path, v0.getX(), v0.getY(), h - r, -pi, -pi + alpha);
                addConditionalArc(path, v2.getX(), v2.getY(), r, -pi + alpha, -pi - halfPi);
                path.closeSubPath();
            }
        }
    }

    return path;
}

juce::Path NotePathManager::getPath(const Note &note, float barWidthPx, float octaveHeightPx,
                                    float noteHeight, float noteRoundCoef) const {
    const int durationScaledPx = juce::roundToInt(note.duration * barWidthPx * MAX_PX_SCALE);
    CacheKey key{durationScaledPx, note.bend};

    auto it = cache.find(key);

    if (it == cache.end()) {
        if (cache.size() >= MAX_CACHE_SIZE) {
            cache.clear();
        }

        juce::Path canonical =
            buildCanonicalPath(note.duration * barWidthPx, note.bend / 1200.0f * octaveHeightPx,
                               noteHeight, noteRoundCoef);

        auto insertion = cache.emplace(key, std::move(canonical));
        it = insertion.first;
    }

    const float x1 = note.time * barWidthPx;
    const float y1 = (numOctaves - note.octave - note.cents / 1200.0f) * octaveHeightPx;

    juce::Path result(it->second);
    result.applyTransform(juce::AffineTransform::translation(x1, y1));

    return result;
}

} // namespace audio_plugin