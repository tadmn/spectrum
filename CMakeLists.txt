cmake_minimum_required(VERSION 3.28)

include(cmake/CPM.cmake)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # Use static runtimes on Windows
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
set(CMAKE_OSX_ARCHITECTURES "arm64") # Only support Apple silicon
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(spectrum VERSION 2.0.0 LANGUAGES C CXX)

option(USE_SANITIZER "Build and link with the address sanitizer" OFF)
option(BUILD_TESTS "Build unit test executable" OFF)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE unspecified; picking Release")
    set(CMAKE_BUILD_TYPE "Release")
endif ()

CPMAddPackage("gh:tadmn/tad-bits#1fc9790bdeed532f50b4e9c31704821d26628969")
include(${tad-bits_SOURCE_DIR}/cmake/compile-options.cmake)

CPMAddPackage("gh:tadmn/choc#5685fb59db9e60d31a35df7bff0ce7967beaaf25")
CPMAddPackage("gh:tadmn/farbot#3cf08080cef9e7c0347fdc144e81b02f5e27a8ca")
CPMAddPackage("gh:tadmn/FastFourier#e2e4ac1660471cbcefb65985b7db27dd6fdea01b")

# Put the analyzer processor into a lib so it can easily be harnessed into the testrunner
add_library(spectrum-analyzer-processor STATIC
        source/analyzer/AnalyzerProcessor.cpp
        source/analyzer/AnalyzerProcessor.h
)

target_link_libraries(spectrum-analyzer-processor PUBLIC tad-bits choc farbot FastFourier)
target_include_directories(spectrum-analyzer-processor PUBLIC source/analyzer)
add_compiler_warnings(spectrum-analyzer-processor)

set(plugin_name "Spectrum")

CPMAddPackage("gh:free-audio/clap#1.2.10")
CPMAddPackage("gh:free-audio/clap-helpers#629ae7526740e3d0ee4d64c230df183ecdb2ff5a")

set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE)
set(CLAP_WRAPPER_OUTPUT_NAME "${plugin_name}")
CPMAddPackage("gh:free-audio/clap-wrapper@0.15.1")

CPMAddPackage("gh:tadmn/visage#a118633d96deffe8c108d34f843715590d283a23")
CPMAddPackage("gh:Neargye/magic_enum@0.9.8")
CPMAddPackage("gh:nlohmann/json@3.11.3")

file(GLOB_RECURSE font_files fonts/*.ttf)
add_embedded_resources(embedded-font-resources "Fonts.h" "resources::fonts" "${font_files}")

file(GLOB_RECURSE shader_files shaders/[vf]s_*.sc)
visage_embed_shaders(embedded-shader-resources "Shaders.h" "resources::shaders" "${shader_files}")

file(GLOB_RECURSE source_files source/plugin/*.cpp source/plugin/*.h)
add_library(spectrum-impl STATIC ${source_files})

target_link_libraries(spectrum-impl PUBLIC clap)
target_link_libraries(spectrum-impl PRIVATE
        spectrum-analyzer-processor
        clap-helpers clap-wrapper-extensions
        visage embedded-font-resources embedded-shader-resources
        magic_enum nlohmann_json
)

target_compile_definitions(spectrum-impl PRIVATE
        PLUGIN_NAME="${plugin_name}"
        PRODUCT_VERSION="${PROJECT_VERSION}"
)

add_compiler_warnings(spectrum-impl)

make_clapfirst_plugins(
        TARGET_NAME spectrum
        IMPL_TARGET spectrum-impl

        OUTPUT_NAME "${plugin_name}"

        ENTRY_SOURCE source/plugin/SpectrumPluginEntry.cpp

        BUNDLE_IDENTIFIER "com.tadmn.spectrum"
        BUNDLE_VERSION ${PROJECT_VERSION}

        COPY_AFTER_BUILD ON

        PLUGIN_FORMATS CLAP VST3 AUV2

        WINDOWS_FOLDER_VST3 OFF

        ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts

        AUV2_MANUFACTURER_NAME "tadmn"
        AUV2_MANUFACTURER_CODE "TadN"
        AUV2_SUBTYPE_CODE "SpCt"
        AUV2_INSTRUMENT_TYPE "aufx"
)

# Ad hoc sign each bundle in the artifacts dir and in the install dir. Without this, hardened hosts such as
# Ableton Live silently reject the plugin
if (APPLE)
    set(spectrum_plugin_targets spectrum_clap spectrum_vst3 spectrum_auv2)
    set(spectrum_plugin_install_dirs
            "$ENV{HOME}/Library/Audio/Plug-Ins/CLAP"
            "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
            "$ENV{HOME}/Library/Audio/Plug-Ins/Components"
    )
    set(spectrum_plugin_extensions clap vst3 component)

    list(LENGTH spectrum_plugin_targets spectrum_plugin_count)
    math(EXPR spectrum_plugin_last "${spectrum_plugin_count} - 1")

    foreach (idx RANGE ${spectrum_plugin_last})
        list(GET spectrum_plugin_targets ${idx} target)
        if (TARGET ${target})
            list(GET spectrum_plugin_install_dirs ${idx} install_dir)
            list(GET spectrum_plugin_extensions ${idx} ext)
            add_custom_command(
                    TARGET ${target} POST_BUILD
                    COMMAND codesign --force --deep --sign - "${CMAKE_BINARY_DIR}/artifacts/${plugin_name}.${ext}"
                    COMMAND codesign --force --deep --sign - "${install_dir}/${plugin_name}.${ext}"
                    COMMENT "Ad-hoc signing ${target} bundle (artifact and install locations)"
            )
        endif ()
    endforeach ()
endif ()

# Suppress warnings in third-party VST3 SDK
if (TARGET base-sdk-vst3)
    if (NOT WIN32)
        target_compile_options(base-sdk-vst3 PRIVATE
                -Wno-nontrivial-memaccess
                -Wno-implicit-const-int-float-conversion
        )
    endif ()
endif ()

if (BUILD_TESTS)
    CPMAddPackage("gh:catchorg/Catch2@3.8.1")
    set(test_runner testrunner)
    add_executable(${test_runner} tests/AnalyzerProcessorTests.cpp)
    target_link_libraries(${test_runner} PRIVATE spectrum-analyzer-processor Catch2::Catch2WithMain)
    target_include_directories(${test_runner} PRIVATE source/analyzer)
    add_compiler_warnings(${test_runner})
endif ()