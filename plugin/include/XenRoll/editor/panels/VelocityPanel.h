#pragma once

#include "XenRoll/data/Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace audio_plugin {
class VeolcitySliderLookAndFeel : public juce::LookAndFeel_V4 {
  public:
    VeolcitySliderLookAndFeel(Theme &theme) : theme(theme) { updateColors(); }

    void updateColors() {
        setColour(juce::Slider::backgroundColourId, theme.darkest);
        setColour(juce::Slider::thumbColourId, theme.brighter);
        setColour(juce::Slider::trackColourId, theme.darkest);

        setColour(juce::Slider::textBoxTextColourId, theme.darkest);
        setColour(juce::Slider::textBoxBackgroundColourId, theme.brighter);
        setColour(juce::Slider::textBoxOutlineColourId, theme.brighter);

        setColour(juce::TextEditor::highlightedTextColourId, theme.darkest);

        setColour(juce::Label::textWhenEditingColourId, theme.darkest);
    }

    juce::Label *createSliderTextBox(juce::Slider &slider) override {
        auto *label = LookAndFeel_V4::createSliderTextBox(slider);
        label->setFont(label->getFont().withHeight(Theme::small_));
        return label;
    }

  private:
    Theme &theme;
};

/**
 * @brief A very small window for changing the velocity of the selected notes(s)
 */
class VelocityPanel : public juce::Component {
  public:
    VelocityPanel(Theme &theme, std::function<void()> onValueChange,
                  std::function<void()> onDragEnd)
        : theme(theme) {
        setWantsKeyboardFocus(false);

        velocitySlider = std::make_unique<juce::Slider>();
        velSliderLF = std::make_unique<VeolcitySliderLookAndFeel>(theme);
        velocitySlider->setLookAndFeel(velSliderLF.get());
        velocitySlider->setRange(0.0, 1.0, 1.0 / 127);
        velocitySlider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, sliderTextBoxWidthPx, 22);

        // to show 0-127 instead of 0.00-1.00 (supports both point and range text formats)
        velocitySlider->textFromValueFunction = [this](double value) {
            if (velocitySlider->getSliderStyle() == juce::Slider::TwoValueHorizontal) {
                int minV = juce::roundToInt(velocitySlider->getMinValue() * 127);
                int maxV = juce::roundToInt(velocitySlider->getMaxValue() * 127);
                return juce::String(minV) + "-" + juce::String(maxV);
            }
            return juce::String(juce::roundToInt(value * 127));
        };

        velocitySlider->valueFromTextFunction = [](const juce::String &text) {
            return text.getDoubleValue() / 127.0;
        };

        velocitySlider->addMouseListener(this, true);
        velocitySlider->onValueChange = [this, onValueChange] {
            if (velocitySlider->getSliderStyle() == juce::Slider::TwoValueHorizontal) {
                // This is only triggered when the user types a single number into the text
                //      box while it's a range, because of onMouseDown()
                double enteredValue = velocitySlider->getValue();
                velocitySlider->setSliderStyle(juce::Slider::LinearHorizontal);
                velocitySlider->setValue(enteredValue, juce::dontSendNotification);
                velocitySlider->updateText();
            }
            onValueChange();
        };
        velocitySlider->onDragEnd = onDragEnd;
        addAndMakeVisible(velocitySlider.get());
    }

    ~VelocityPanel() { velocitySlider->setLookAndFeel(nullptr); }

    void resized() override {
        auto bounds = getLocalBounds();
        bounds.removeFromBottom(2);
        bounds.removeFromTop(2);
        bounds.removeFromLeft(10);
        bounds.removeFromRight(4);
        velocitySlider->setBounds(bounds);
    }

    ///< velocity in range [0; 1]
    void setVelocityRange(float minVel, float maxVel) {
        if (std::abs(minVel - maxVel) < 0.001) {
            // Single point
            velocitySlider->setSliderStyle(juce::Slider::LinearHorizontal);
            velocitySlider->setValue(minVel, juce::dontSendNotification);
        } else {
            // Range
            velocitySlider->setSliderStyle(juce::Slider::TwoValueHorizontal);
            velocitySlider->setMinAndMaxValues(minVel, maxVel, juce::dontSendNotification);
        }
        velocitySlider->updateText();
        repaint();
    }

    ///< velocity in range [0; 1]
    float getVelocity() {
        // single point mode
        return static_cast<float>(velocitySlider->getValue());
    }

    void paint(juce::Graphics &g) override {
        g.setColour(theme.bright);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);
    }

    void updateColors() {
        velSliderLF->updateColors();
        repaint();
    }

  protected:
    // This catches the mouse down event BEFORE JUCE initializes the drag constraints
    void mouseDown(const juce::MouseEvent &event) override {
        // range mode -> point mode
        if (event.originalComponent == velocitySlider.get() &&
            velocitySlider->getSliderStyle() == juce::Slider::TwoValueHorizontal) {

            auto sliderRect =
                velocitySlider->getLookAndFeel().getSliderLayout(*velocitySlider).sliderBounds;
            double mouseX = (double)event.x;
            double trackLeft = (double)sliderRect.getX();
            double trackWidth = (double)sliderRect.getWidth();
            double mouseXInTrack = mouseX - trackLeft;
            double clickProportion = mouseXInTrack / trackWidth;
            clickProportion = juce::jlimit(0.0, 1.0, clickProportion);

            double newValue =
                velocitySlider->getMinimum() +
                (clickProportion * (velocitySlider->getMaximum() - velocitySlider->getMinimum()));

            velocitySlider->setSliderStyle(juce::Slider::LinearHorizontal);
            velocitySlider->setValue(newValue, juce::dontSendNotification);

            // need mouseUp & mouseDown to remove restrictions that are set in range mode
            velocitySlider->mouseUp(event);
            velocitySlider->mouseDown(event);
        }
    }

  private:
    Theme &theme;
    std::unique_ptr<juce::Slider> velocitySlider;
    std::unique_ptr<VeolcitySliderLookAndFeel> velSliderLF;

    const int sliderTextBoxWidthPx = 70;
    const float cornerSize = 10.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelocityPanel)
};
} // namespace audio_plugin