#pragma once

#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h> // for juce::Decibels
#include <juce_data_structures/juce_data_structures.h>

namespace audio_plugin {
///< Class for global settings (same for ALL instances in all projects)
class GlobalSettings {
  public:
    static GlobalSettings &getInstance() {
        static GlobalSettings instance;
        return instance;
    }

    //  ============ playDraggedNotes ============
    static constexpr bool playDraggedNotes_default = true;
    bool getPlayDraggedNotes() const { return playDraggedNotes_.load(std::memory_order_relaxed); }
    void setPlayDraggedNotes(bool playDraggedNotes) {
        playDraggedNotes_.store(playDraggedNotes, std::memory_order_relaxed);
        propsFile->setValue("playDraggedNotes", playDraggedNotes);
        propsFile->saveIfNeeded();
    } // =========================================

    //  ============ chaseMIDINotes ============
    static constexpr bool chaseMIDINotes_default = true;
    bool getChaseMIDINotes() const { return chaseMIDINotes_.load(std::memory_order_relaxed); }
    void setChaseMIDINotes(bool chaseMIDINotes) {
        chaseMIDINotes_.store(chaseMIDINotes, std::memory_order_relaxed);
        propsFile->setValue("chaseMIDINotes", chaseMIDINotes);
        propsFile->saveIfNeeded();
    } // =========================================

    //  ============ horZoomOnCursor ============
    static constexpr bool horZoomOnCursor_default = true;
    bool getHorZoomOnCursor() const { return horZoomOnCursor_.load(std::memory_order_relaxed); }
    void setHorZoomOnCursor(bool horZoomOnCursor) {
        horZoomOnCursor_.store(horZoomOnCursor, std::memory_order_relaxed);
        propsFile->setValue("horZoomOnCursor", horZoomOnCursor);
        propsFile->saveIfNeeded();
    } // =========================================

    //  =============== micGain_dB ===============
    static constexpr double min_micGain_dB = -24.0;
    static constexpr double max_micGain_dB = 24.0;
    static constexpr double micGain_dB_default = 0.0;
    double getMicGain_dB() const { return micGain_dB_.load(std::memory_order_relaxed); }
    ///< Returns pre-calculated linear gain (good for audio thread)
    float getMicGainLinear() const { return micGainLinear_.load(std::memory_order_relaxed); }
    void setMicGain_dB(double micGain_dB) {
        micGain_dB_.store(micGain_dB, std::memory_order_relaxed);
        micGainLinear_.store(juce::Decibels::decibelsToGain(micGain_dB), std::memory_order_relaxed);
        propsFile->setValue("micGain_dB", micGain_dB);
        propsFile->saveIfNeeded();
    } // =========================================

    //  =============== noteRectRounding =========
    static constexpr double min_noteRectRounding = 0.0;
    static constexpr double max_noteRectRounding = 1.0;
    static constexpr double noteRectRounding_default = 0.0;
    double getNoteRectRounding() const { return noteRectRounding_.load(std::memory_order_relaxed); }
    void setNoteRectRounding(double noteRectRounding) {
        noteRectRounding_.store(noteRectRounding, std::memory_order_relaxed);
        propsFile->setValue("noteRectRounding", noteRectRounding);
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

        reloadCache();
    }

    // The destructor that prevents jassert when unloading the DLL
    ~GlobalSettings() {
        // Releasing ownership so as not to call the PropertiesFile destructor,
        // which can access already destroyed JUCE timers.
        // A memory leak is safe here because the process is terminating.
        propsFile.release();
    }

    void reloadCache() {
        playDraggedNotes_.store(
            propsFile->getBoolValue("playDraggedNotes", playDraggedNotes_default),
            std::memory_order_relaxed);

        chaseMIDINotes_.store(propsFile->getBoolValue("chaseMIDINotes", chaseMIDINotes_default),
                              std::memory_order_relaxed);

        horZoomOnCursor_.store(propsFile->getBoolValue("horZoomOnCursor", horZoomOnCursor_default),
                               std::memory_order_relaxed);

        double db = propsFile->getDoubleValue("micGain_dB", micGain_dB_default);
        micGain_dB_.store(db, std::memory_order_relaxed);
        micGainLinear_.store(juce::Decibels::decibelsToGain(db), std::memory_order_relaxed);

        noteRectRounding_.store(
            propsFile->getDoubleValue("noteRectRounding", noteRectRounding_default),
            std::memory_order_relaxed);
    }

    std::unique_ptr<juce::PropertiesFile> propsFile;

    //  ============ Cached values (atomic, lock-free) ============
    std::atomic<bool> playDraggedNotes_;
    std::atomic<bool> chaseMIDINotes_;
    std::atomic<bool> horZoomOnCursor_;
    std::atomic<double> micGain_dB_;
    std::atomic<float> micGainLinear_;
    std::atomic<double> noteRectRounding_;
    // ===========================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalSettings)
};
} // namespace audio_plugin