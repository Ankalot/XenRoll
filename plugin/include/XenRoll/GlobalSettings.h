#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
///< Class for global settings (same for ALL instances in all projects)
class GlobalSettings {
  public:
    static GlobalSettings &getInstance() {
        static GlobalSettings instance;
        return instance;
    }

    //  ============ playDraggedNotes ============
    bool getPlayDraggedNotes() const { return propsFile->getBoolValue("playDraggedNotes", true); }
    void setPlayDraggedNotes(bool playDraggedNotes) {
        propsFile->setValue("playDraggedNotes", playDraggedNotes);
        propsFile->saveIfNeeded();
    } // =========================================

    //  =============== micGain_dB ===============
    static constexpr float min_micGain_dB = -24.0;
    static constexpr float max_micGain_dB = 24.0;
    double getMicGain_dB() const { return propsFile->getDoubleValue("micGain_dB", 0.0); }
    void setMicGain_dB(double micGain_dB) {
        propsFile->setValue("micGain_dB", micGain_dB);
        propsFile->saveIfNeeded();
    } // =========================================

  private:
    GlobalSettings() {
        auto settingsFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                .getChildFile("Ankalot")
                                .getChildFile("XenRoll")
                                .getChildFile("global_settings.xml");
        settingsFile.getParentDirectory().createDirectory();

        juce::PropertiesFile::Options options;
        options.applicationName = "XenRoll";
        options.filenameSuffix = ".xml";
        options.osxLibrarySubFolder = "Application Support/Ankalot/XenRoll";
        options.storageFormat = juce::PropertiesFile::storeAsXML;

        propsFile = std::make_unique<juce::PropertiesFile>(settingsFile, options);
    }

    // The destructor that prevents jassert when unloading the DLL
    ~GlobalSettings() {
        // Releasing ownership so as not to call the PropertiesFile destructor,
        // which can access already destroyed JUCE timers.
        // A memory leak is safe here because the process is terminating.
        propsFile.release();
    }

    std::unique_ptr<juce::PropertiesFile> propsFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalSettings)
};
} // namespace audio_plugin