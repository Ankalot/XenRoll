#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class SVGButton : public juce::Component, public juce::TooltipClient {
  public:
    SVGButton(const char *svgBinaryData, int svgBinaryDataSize, bool togglable = true,
              bool active = false, const std::string &tooltipText = "")
        : togglable(togglable), active(active) {
        this->tooltipText = juce::String(tooltipText);
        auto svgXml =
            juce::XmlDocument::parse(juce::String::fromUTF8(svgBinaryData, svgBinaryDataSize));
        svg = juce::Drawable::createFromSVG(*svgXml);
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    SVGButton(const juce::String &svgData, bool togglable = true, bool active = false,
              const std::string &tooltipText = "")
        : togglable(togglable), active(active) {
        this->tooltipText = juce::String(tooltipText);
        svg = juce::Drawable::createFromSVG(*juce::XmlDocument::parse(svgData));
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void paint(juce::Graphics &g) override {
        if (togglable && active) {
            recolorDrawable(svg.get(), Theme::activated);
        } else {
            recolorDrawable(svg.get(), Theme::bright);
        }

        if (svg != nullptr) {
            auto bounds = getLocalBounds().toFloat();
            auto svgBounds = svg->getDrawableBounds();

            float scale = juce::jmin(bounds.getWidth() / svgBounds.getWidth(),
                                     bounds.getHeight() / svgBounds.getHeight());

            auto transform = juce::AffineTransform::scale(scale);
            transform = transform.translated(bounds.getCentre() - svgBounds.getCentre() * scale);

            svg->draw(g, 1.0f, transform);
        }
    }

    void resized() override { repaint(); }

    std::function<bool(const juce::MouseEvent &me)> onClick;

    void triggerLeftClick() {
        if (onClick) {
            auto center = juce::Point<float>(getWidth() / 2.0f, getHeight() / 2.0f);

            auto &desktop = juce::Desktop::getInstance();
            if (desktop.getNumMouseSources() > 0) {
                auto *mouseSource = desktop.getMouseSource(0);

                if (mouseSource != nullptr) {
                    juce::MouseEvent fakeEvent(
                        *mouseSource, center, juce::ModifierKeys::leftButtonModifier, 1.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, this, this, juce::Time::getCurrentTime(), center,
                        juce::Time::getCurrentTime(), 1, false);

                    mouseDown(fakeEvent);
                }
            }
        }
    }

    juce::String getTooltip() override { return tooltipText; }

    void mouseEnter(const juce::MouseEvent &) override {
        repaint();
        juce::Desktop::getInstance().getMainMouseSource().forceMouseCursorUpdate();
    }

    void mouseExit(const juce::MouseEvent &) override { repaint(); }

    void mouseDown(const juce::MouseEvent &me) override {
        if (onClick) {
            bool changeState = onClick(me);
            if (changeState) {
                active = !active;
                repaint();
            }
        }
    }

    void mouseUp(const juce::MouseEvent &) override { repaint(); }

    bool isActive() { return active; }

    //void setState(bool activeNew) { active = activeNew; }

  private:
    std::unique_ptr<juce::Drawable> svg;
    juce::String tooltipText;
    bool togglable;
    bool active;

    void recolorDrawable(juce::Drawable *drawable, const juce::Colour &newColor) {
        if (auto *shape = dynamic_cast<juce::DrawableShape *>(drawable)) {
            shape->setFill(newColor);
            shape->setStrokeFill(newColor);
        } else if (auto *group = dynamic_cast<juce::DrawableComposite *>(drawable)) {
            for (auto *child : group->getChildren())
                if (auto *childDrawable = dynamic_cast<juce::Drawable *>(child))
                    recolorDrawable(childDrawable, newColor);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SVGButton)
};
} // namespace audio_plugin