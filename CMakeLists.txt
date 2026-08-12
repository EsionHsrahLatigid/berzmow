cmake_minimum_required(VERSION 3.22)

project(berzmow VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BERZMOW_BUILD_PLUGIN "Build the JUCE plug-in targets" ON)
option(BERZMOW_BUILD_TESTS "Build processor integration tests" ON)
set(BERZMOW_ARTIFACT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/artifacts" CACHE PATH
    "Directory for final VST3, AU, and Standalone products")
set(EHL_JUCE_SOURCE_DIR "" CACHE PATH "Existing JUCE source checkout; avoids network when set")

if(BERZMOW_BUILD_TESTS)
    enable_testing()
endif()

if(BERZMOW_BUILD_PLUGIN)
    include(FetchContent)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    if(EHL_JUCE_SOURCE_DIR)
        if(NOT EXISTS "${EHL_JUCE_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "EHL_JUCE_SOURCE_DIR does not contain JUCE: ${EHL_JUCE_SOURCE_DIR}")
        endif()
        add_subdirectory("${EHL_JUCE_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/juce")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
        add_subdirectory(JUCE)
    else()
        FetchContent_Declare(
            JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 8.0.15
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(JUCE)
    endif()
    add_subdirectory(modules/juce-ehl-design-module EXCLUDE_FROM_ALL)

    set(BERZMOW_FORMATS VST3 Standalone)
    if(APPLE)
        list(APPEND BERZMOW_FORMATS AU)
    endif()

    juce_add_plugin(Berzmow
        VERSION ${PROJECT_VERSION}
        COMPANY_NAME "EsionHsrahLatigid"
        BUNDLE_ID "jp.ehl.berzmow"
        PRODUCT_NAME "Berzmow"
        DESCRIPTION "Feedback-driven noise terror distortion effect"
        PLUGIN_MANUFACTURER_CODE EHL_
        PLUGIN_CODE berz
        FORMATS ${BERZMOW_FORMATS}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Fx Distortion
        AU_MAIN_TYPE kAudioUnitType_Effect)

    juce_generate_juce_header(Berzmow)

    target_sources(Berzmow PRIVATE
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h)

    target_compile_definitions(Berzmow PUBLIC
        JUCE_REPORT_APP_USAGE=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0)

    target_link_libraries(Berzmow
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            EHL::JuceDesign
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    set(berzmow_stage_platform "linux-x64")
    if(APPLE)
        set(berzmow_stage_platform "macos-arm64")
    elseif(WIN32)
        set(berzmow_stage_platform "windows-x64")
    endif()
    set(berzmow_juce_products "${CMAKE_CURRENT_BINARY_DIR}/Berzmow_artefacts/$<CONFIG>")
    set(berzmow_staged_products "${BERZMOW_ARTIFACT_DIR}/plugin-release/${berzmow_stage_platform}")
    set(berzmow_artifact_dependencies Berzmow_VST3 Berzmow_Standalone)
    set(berzmow_artifact_commands
        COMMAND ${CMAKE_COMMAND} -E make_directory "${berzmow_staged_products}/vst3"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${berzmow_staged_products}/standalone"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${berzmow_staged_products}/au")

    if(APPLE)
        list(APPEND berzmow_artifact_commands
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${berzmow_staged_products}/vst3/berzmow_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${berzmow_staged_products}/standalone/berzmow_standalone_plugin.app"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${berzmow_staged_products}/au/berzmow_au_plugin.component"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${berzmow_juce_products}/VST3/Berzmow.vst3" "${berzmow_staged_products}/vst3/berzmow_vst3_plugin.vst3"
            COMMAND ${Python3_EXECUTABLE}
                    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NormalizeVst3ModuleInfo.py"
                    "${berzmow_staged_products}/vst3/berzmow_vst3_plugin.vst3/Contents/Resources/moduleinfo.json"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${berzmow_juce_products}/Standalone/Berzmow.app" "${berzmow_staged_products}/standalone/berzmow_standalone_plugin.app")
    elseif(WIN32)
        list(APPEND berzmow_artifact_commands
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${berzmow_staged_products}/vst3/berzmow_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${berzmow_juce_products}/VST3/Berzmow.vst3" "${berzmow_staged_products}/vst3/berzmow_vst3_plugin.vst3"
            COMMAND ${Python3_EXECUTABLE}
                    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NormalizeVst3ModuleInfo.py"
                    "${berzmow_staged_products}/vst3/berzmow_vst3_plugin.vst3/Contents/Resources/moduleinfo.json"
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${berzmow_juce_products}/Standalone/Berzmow.exe" "${berzmow_staged_products}/standalone/berzmow_standalone_plugin.exe")
    else()
        list(APPEND berzmow_artifact_commands
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${berzmow_staged_products}/vst3/berzmow_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${berzmow_juce_products}/VST3/Berzmow.vst3" "${berzmow_staged_products}/vst3/berzmow_vst3_plugin.vst3"
            COMMAND ${Python3_EXECUTABLE}
                    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NormalizeVst3ModuleInfo.py"
                    "${berzmow_staged_products}/vst3/berzmow_vst3_plugin.vst3/Contents/Resources/moduleinfo.json"
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${berzmow_juce_products}/Standalone/Berzmow" "${berzmow_staged_products}/standalone/berzmow_standalone_plugin")
    endif()

    if(TARGET Berzmow_AU)
        list(APPEND berzmow_artifact_dependencies Berzmow_AU)
        list(APPEND berzmow_artifact_commands
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${berzmow_juce_products}/AU/Berzmow.component" "${berzmow_staged_products}/au/berzmow_au_plugin.component")
    endif()

    if(APPLE)
        list(APPEND berzmow_artifact_commands
            COMMAND codesign --force --deep --sign -
                    "${berzmow_staged_products}/vst3/berzmow_vst3_plugin.vst3"
            COMMAND codesign --force --deep --sign -
                    "${berzmow_staged_products}/standalone/berzmow_standalone_plugin.app"
            COMMAND codesign --force --deep --sign -
                    "${berzmow_staged_products}/au/berzmow_au_plugin.component")
    endif()

    list(APPEND berzmow_artifact_commands
        COMMAND ${CMAKE_COMMAND}
            -DPRODUCT_NAME=Berzmow
            -DSLUG=berzmow
            -DBUNDLE_ID=jp.ehl.berzmow
            -DPLUGIN_CODE=berz
            -DSTAGE_DIR=${berzmow_staged_products}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteArtifactsManifest.cmake")

    add_custom_target(ehl_stage_products
        ${berzmow_artifact_commands}
        DEPENDS ${berzmow_artifact_dependencies}
        COMMENT "Staging Berzmow products under ${BERZMOW_ARTIFACT_DIR}/plugin-release")

    add_custom_target(Berzmow_Artifacts DEPENDS ehl_stage_products)

    if(BERZMOW_BUILD_TESTS)
        add_executable(BerzmowIntegrationTests Tests/PluginIntegrationTest.cpp)
        target_include_directories(BerzmowIntegrationTests PRIVATE
            Source
            "${CMAKE_CURRENT_BINARY_DIR}/Berzmow_artefacts/JuceLibraryCode")
        target_link_libraries(BerzmowIntegrationTests PRIVATE
            Berzmow
            EHL::JuceDesign
            juce::juce_audio_utils)
        add_dependencies(BerzmowIntegrationTests Berzmow_Artifacts)
        add_test(NAME BerzmowPluginIntegrationTests COMMAND BerzmowIntegrationTests)
        add_test(
            NAME BerzmowArtifactChecks
            COMMAND ${CMAKE_COMMAND}
                    -DSTAGE_DIR=${berzmow_staged_products}
                    -DSLUG=berzmow
                    -DEXPECT_AU=$<BOOL:${APPLE}>
                    -DPython3_EXECUTABLE=${Python3_EXECUTABLE}
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/Tests/VerifyArtifacts.cmake)
    endif()
endif()
