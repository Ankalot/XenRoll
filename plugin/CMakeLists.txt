cmake_minimum_required(VERSION 3.22)

# Version is needed by JUCE.
project(XenRoll VERSION 0.4.2)

# Adding a directory with the library/application name as a subfolder of the
# include folder is a good practice. It helps avoid name clashes later on.
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/XenRoll")

set(EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
include_directories(${EXTERNAL_DIR})

# When building I am hanging on creating moduleinfo.json (Windows...) so i disabled it
# set(JUCE_VST3_CREATE_MANIFEST_FILE OFF)

# Adds a plugin target (that's basically what the Projucer does).
juce_add_plugin(
    ${PROJECT_NAME}
    COMPANY_NAME
    "Ankalot"
    IS_SYNTH
    FALSE
    NEEDS_MIDI_INPUT
    TRUE
    NEEDS_MIDI_OUTPUT
    TRUE
    PLUGIN_MANUFACTURER_CODE
    Ankt
    PLUGIN_CODE
    Xnrl
    FORMATS
    VST3
    PRODUCT_NAME
    "XenRoll"
    LV2URI "https://github.com/ankalot/xenroll" # for linux lv2
)

# Disable some warnings from boost lib
if(WIN32)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4619)
endif()

# Enable Address Sanitizer for Debug builds on Windows/MSVC
#[[if(WIN32 AND MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
    target_link_options(${PROJECT_NAME} PRIVATE /fsanitize=address)

    # Optional: Set ASAN options for better output (can also be set via environment variable)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)

    message(STATUS "Address Sanitizer is ENABLED for Debug build")
endif()]]

# Sets the source files of the plugin project.
set(SOURCE_FILES

    # common
    source/common/PlatformUtils.cpp

    # data
    source/data/RatioMark.cpp
    source/data/Theme.cpp

    # editor
    source/editor/PluginEditor.cpp

    # editor/menus
    source/editor/menus/ClockDiagramMenu.cpp
    source/editor/menus/EditRatiosMarksMenu.cpp
    source/editor/menus/GenNewKeysMenu.cpp
    source/editor/menus/InstancesMenu.cpp
    source/editor/menus/MoreToolsMenu.cpp
    source/editor/menus/VocalToMelodyMenu.cpp

    # editor/models
    source/editor/models/DissonanceMeter.cpp
    source/editor/models/PitchMemory.cpp

    # editor/panels
    source/editor/panels/ClockDiagramPanel.cpp
    source/editor/panels/DissonancePanel.cpp
    source/editor/panels/LeftPanel.cpp
    source/editor/panels/MainPanel.cpp
    source/editor/panels/NotePathManager.cpp
    source/editor/panels/PitchMemorySettingsPanel.cpp
    source/editor/panels/SettingsPanel.cpp
    source/editor/panels/TopPanel.cpp

    # processor
    source/processor/PluginProcessor.cpp

    # processor/audio/dsp
    source/processor/audio/dsp/PartialsFinder.cpp
    source/processor/audio/dsp/PitchDetectorMPM.cpp

    # processor/managers
    source/processor/managers/NotesSharingMPE.cpp
    source/processor/managers/PluginInstanceManager.cpp

    # external
    ${EXTERNAL_DIR}/MTS-ESP/libMTSMaster.cpp
)

# Optional; includes header files in the project file tree in Visual Studio
set(HEADER_FILES

    # common
    ${INCLUDE_DIR}/common/CircularStack.h
    ${INCLUDE_DIR}/common/PlatformUtils.h
    ${INCLUDE_DIR}/common/RelevanceQueue.h

    # data
    ${INCLUDE_DIR}/data/GlobalSettings.h
    ${INCLUDE_DIR}/data/Note.h
    ${INCLUDE_DIR}/data/Parameters.h
    ${INCLUDE_DIR}/data/PartialsTypes.h
    ${INCLUDE_DIR}/data/RatioMark.h
    ${INCLUDE_DIR}/data/Theme.h
    ${INCLUDE_DIR}/data/Zones.h

    # editor
    ${INCLUDE_DIR}/editor/DragAndDropPopup.h
    ${INCLUDE_DIR}/editor/PluginEditor.h

    # editor/common
    ${INCLUDE_DIR}/editor/common/IntegerInput.h
    ${INCLUDE_DIR}/editor/common/PopupMessage.h
    ${INCLUDE_DIR}/editor/common/SVGButton.h

    # editor/menus
    ${INCLUDE_DIR}/editor/menus/ClockDiagramMenu.h
    ${INCLUDE_DIR}/editor/menus/EditRatiosMarksMenu.h
    ${INCLUDE_DIR}/editor/menus/GenNewKeysMenu.h
    ${INCLUDE_DIR}/editor/menus/InstancesMenu.h
    ${INCLUDE_DIR}/editor/menus/MoreToolsMenu.h
    ${INCLUDE_DIR}/editor/menus/VocalToMelodyMenu.h

    # editor/models
    ${INCLUDE_DIR}/editor/models/DissonanceMeter.h
    ${INCLUDE_DIR}/editor/models/PitchMemory.h

    # editor/panels
    ${INCLUDE_DIR}/editor/panels/ClockDiagramPanel.h
    ${INCLUDE_DIR}/editor/panels/DissonancePanel.h
    ${INCLUDE_DIR}/editor/panels/DissonancePlot.h
    ${INCLUDE_DIR}/editor/panels/HelpPanel.h
    ${INCLUDE_DIR}/editor/panels/LeftPanel.h
    ${INCLUDE_DIR}/editor/panels/MainPanel.h
    ${INCLUDE_DIR}/editor/panels/NotePathManager.h
    ${INCLUDE_DIR}/editor/panels/PartialsPlot.h
    ${INCLUDE_DIR}/editor/panels/PitchMemorySettingsPanel.h
    ${INCLUDE_DIR}/editor/panels/SettingsPanel.h
    ${INCLUDE_DIR}/editor/panels/TopPanel.h
    ${INCLUDE_DIR}/editor/panels/VelocityPanel.h

    # processor
    ${INCLUDE_DIR}/processor/PluginProcessor.h

    # processor/audio
    ${INCLUDE_DIR}/processor/audio/AccumulatingBuffer.h

    # processor/audio/dsp
    ${INCLUDE_DIR}/processor/audio/dsp/PartialsFinder.h
    ${INCLUDE_DIR}/processor/audio/dsp/PitchDetectorMPM.h

    # processor/managers
    ${INCLUDE_DIR}/processor/managers/ChannelsManagerMPE.h
    ${INCLUDE_DIR}/processor/managers/NotesSharingMPE.h
    ${INCLUDE_DIR}/processor/managers/PluginInstanceManager.h

    # external
    ${EXTERNAL_DIR}/MTS-ESP/libMTSMaster.h
)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES} ${HEADER_FILES})

# Sets the include directories of the plugin project.
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${MTS_ESP_DIR} ${PITCH_DETECTOR_DIR})

# Links to all necessary dependencies. The present ones are recommended by JUCE.
# If you use one of the additional modules, like the DSP module, you need to specify it here.
find_package(Boost REQUIRED COMPONENTS interprocess)
target_link_libraries_system(
    ${PROJECT_NAME}
    PUBLIC
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_audio_formats
    juce::juce_osc
)
target_link_libraries(
    ${PROJECT_NAME} PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    PRIVATE Boost::interprocess
)

# These definitions are recommended by JUCE.
target_compile_definitions(${PROJECT_NAME} PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

# Enables strict C++ warnings and treats warnings as errors.
# This needs to be set up only for your projects, not 3rd party
set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "${PROJECT_WARNINGS_CXX}")

# In Visual Studio this command provides a nice grouping of source files in "filters".
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/..)

file(GLOB RESOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/CambriaMath.ttf"
)

foreach(resource ${RESOURCE_FILES})
    if(NOT EXISTS ${resource})
        message(FATAL_ERROR "Resource file not found: ${resource}")
    endif()
endforeach()

juce_add_binary_data(
    XenRollFonts
    SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/CambriaMath.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Brain.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Broom.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Export.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Import.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Help.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Fix_cam.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Refresh.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Settings.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Snap_keys.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Snap_time.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Transpose.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Trash.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Turn_on.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Zones_off.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Zones_on.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Partials.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Import_partials.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Export_partials.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Brain_settings.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Keys_harmonicity.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Ghost_notes_keys.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Ghost_notes_tab.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Generate_new_keys.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Hide_cents.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Edit_ratios_marks.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/More_tools.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Notes_from_ghost.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Vocal_to_melody.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Record_notes.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/SVG/Clock_diagram.svg
    HEADER_NAME BinaryData.h
)

# Link fonts to your plugin
target_link_libraries(${PROJECT_NAME} PRIVATE XenRollFonts)