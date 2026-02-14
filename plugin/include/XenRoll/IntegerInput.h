#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class IntegerInput : public juce::Component {
  public:
    /**
     * @brief Construct an IntegerInput component
     * @param val Initial value
     * @param minVal Minimum allowed value
     * @param maxVal Maximum allowed value
     */
    IntegerInput(int val, int minVal, int maxVal) : minVal(minVal), maxVal(maxVal) {
        juce::Font currentFont = editor.getFont();
        currentFont.setHeight(Theme::big);
        editor.setFont(currentFont);

        lastValidValue = val;
        addAndMakeVisible(editor);
        editor.setInputRestrictions((int)ceil(log10(maxVal)), "0123456789");
        setValue(val);

        // Validate when focus is lost
        editor.onFocusLost = [this] {
            int value = editor.getText().getIntValue();
            if (value < this->minVal || value > this->maxVal) {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon, "Invalid Input",
                    "Please enter a whole number between " + juce::String(this->minVal) + " and " +
                        juce::String(this->maxVal),
                    "OK");
                editor.setText(juce::String(lastValidValue), juce::dontSendNotification);
                editor.grabKeyboardFocus();
            } else {
                lastValidValue = value;
                if (onValueChanged)
                    onValueChanged(value);
            }
        };

        // Also validate on return key
        editor.onReturnKey = [this] {
            editor.unfocusAllComponents(); // Triggers onFocusLost
        };
    }

    void resized() override { editor.setBounds(getLocalBounds()); }

    void lookAndFeelChanged() override { editor.applyColourToAllText(Theme::brightest); }

    int getValue() const { return lastValidValue; }

    void setValue(int newValue) {
        newValue = juce::jlimit(minVal, maxVal, newValue);
        editor.setText(juce::String(newValue), juce::dontSendNotification);
        lastValidValue = newValue;
    }

    /**
     * @brief Callback function called when value changes
     */
    std::function<void(int)> onValueChanged;

  private:
    juce::TextEditor editor;
    int lastValidValue;
    const int minVal, maxVal;
};
} // namespace audio_plugin