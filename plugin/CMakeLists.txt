cmake_minimum_required(VERSION 3.22)

# Version is needed by JUCE.
project(XenRoll VERSION 0.2.3)

# Adding a directory with the library/application name as a subfolder of the
# include folder is a good practice. It helps avoid name clashes later on.
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/XenRoll")

set(MTS_ESP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/MTS-ESP")
include_directories(${MTS_ESP_DIR})

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
  LV2URI "https://github.com/ankalot/xenroll" #for linux lv2
)

# Disable some warnings from boost lib
if(WIN32)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4619)
endif()

# Enable Address Sanitizer for Debug builds on Windows/MSVC
if(WIN32 AND MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
    target_link_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
    
    # Optional: Set ASAN options for better output (can also be set via environment variable)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
    
    message(STATUS "Address Sanitizer is ENABLED for Debug build")
endif()

# Sets the source files of the plugin project.
set(SOURCE_FILES source/PluginEditor.cpp 
                 source/PluginProcessor.cpp 
                 source/MainPanel.cpp
                 source/LeftPanel.cpp
                 source/TopPanel.cpp
                 source/SettingsPanel.cpp
                 source/InstancesMenu.cpp
                 source/MoreToolsMenu.cpp
                 source/Theme.cpp
                 source/PlatformUtils.cpp
                 source/PluginInstanceManager.cpp
                 source/DissonancePanel.cpp
                 source/DissonanceMeter.cpp
                 source/PartialsFinder.cpp
                 source/PitchMemory.cpp
                 source/PitchMemorySettingsPanel.cpp
                 source/GenNewKeysMenu.cpp
                 source/EditRatiosMarksMenu.cpp
                 source/RatioMark.cpp
                 ${MTS_ESP_DIR}/libMTSMaster.cpp)
# Optional; includes header files in the project file tree in Visual Studio
set(HEADER_FILES ${INCLUDE_DIR}/PluginEditor.h 
                 ${INCLUDE_DIR}/PluginProcessor.h 
                 ${INCLUDE_DIR}/IntegerInput.h
                 ${INCLUDE_DIR}/Note.h
                 ${INCLUDE_DIR}/RatioMark.h
                 ${INCLUDE_DIR}/CircularStack.h
                 ${INCLUDE_DIR}/RelevanceQueue.h
                 ${INCLUDE_DIR}/TopPanel.h 
                 ${INCLUDE_DIR}/LeftPanel.h 
                 ${INCLUDE_DIR}/MainPanel.h 
                 ${INCLUDE_DIR}/PopupMessage.h
                 ${INCLUDE_DIR}/HelpPanel.h
                 ${INCLUDE_DIR}/Zones.h
                 ${INCLUDE_DIR}/VelocityPanel.h
                 ${INCLUDE_DIR}/SettingsPanel.h
                 ${INCLUDE_DIR}/SVGButton.h
                 ${INCLUDE_DIR}/InstancesMenu.h
                 ${INCLUDE_DIR}/MoreToolsMenu.h
                 ${INCLUDE_DIR}/Theme.h
                 ${INCLUDE_DIR}/PlatformUtils.h
                 ${INCLUDE_DIR}/PluginInstanceManager.h
                 ${INCLUDE_DIR}/DissonancePanel.h
                 ${INCLUDE_DIR}/PartialsPlot.h
                 ${INCLUDE_DIR}/DissonancePlot.h
                 ${INCLUDE_DIR}/DissonanceMeter.h
                 ${INCLUDE_DIR}/PartialsFinder.h
                 ${INCLUDE_DIR}/AccumulatingBuffer.h
                 ${INCLUDE_DIR}/PitchMemorySettingsPanel.h
                 ${INCLUDE_DIR}/PitchMemory.h
                 ${INCLUDE_DIR}/GenNewKeysMenu.h
                 ${INCLUDE_DIR}/EditRatiosMarksMenu.h
                 ${INCLUDE_DIR}/DragAndDropPopup.h
                 ${MTS_ESP_DIR}/libMTSMaster.h)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES} ${HEADER_FILES})

# Sets the include directories of the plugin project.
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${MTS_ESP_DIR})

# Links to all necessary dependencies. The present ones are recommended by JUCE.
# If you use one of the additional modules, like the DSP module, you need to specify it here.
find_package(Boost REQUIRED COMPONENTS interprocess)
target_link_libraries_system(
    ${PROJECT_NAME} 
    PUBLIC 
    juce::juce_audio_utils 
    juce::juce_dsp 
    juce::juce_audio_formats
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
    HEADER_NAME BinaryData.h
)

# Link fonts to your plugin
target_link_libraries(${PROJECT_NAME} PRIVATE XenRollFonts)