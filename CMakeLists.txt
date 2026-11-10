cmake_minimum_required(VERSION 3.28)

include(cmake/CPM.cmake)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # Use static runtimes on Windows
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
set(CMAKE_OSX_ARCHITECTURES "arm64") # Only support Apple silicon
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(spectrum VERSION 2.0.1 LANGUAGES C CXX)

option(USE_SANITIZER "Build and link with the address sanitizer" OFF)
option(BUILD_TESTS "Build unit test executable" OFF)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE unspecified; picking Release")
    set(CMAKE_BUILD_TYPE "Release")
endif ()

# The browser build (see the web/ directory), configured with `emcmake cmake`
if (EMSCRIPTEN)
    # Applies to every dependency, as all code must agree on the exception handling ABI. Exceptions are
    # needed for the state loading to recover from bad JSON
    add_compile_options(-msimd128 -fwasm-exceptions)
    add_link_options(-fwasm-exceptions)

    # choc & visage rely on headers that newer versions of libc++ no longer include transitively
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include cstdlib>"
                        "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include vector>")
endif ()

# Must come before tad-bits, which adds its own older Catch2 when BUILD_TESTS is on and CPM keeps the first one added
if (BUILD_TESTS)
    CPMAddPackage("gh:catchorg/Catch2@3.16.1")
endif ()

CPMAddPackage("gh:tadmn/tad-bits#067cc868dd6d247cfe23a762deae538cb96e546c")
include(${tad-bits_SOURCE_DIR}/cmake/compile-options.cmake)

CPMAddPackage("gh:tadmn/choc#5685fb59db9e60d31a35df7bff0ce7967beaaf25")
CPMAddPackage("gh:tadmn/farbot#3cf08080cef9e7c0347fdc144e81b02f5e27a8ca")
CPMAddPackage("gh:tadmn/FastFourier#2a51af46b9e0d857ab237aeed5f3452a38f22c9e")

# Put the analyzer processor into a lib so it can easily be harnessed into the testrunner
add_library(spectrum-analyzer-processor STATIC
        source/analyzer/AnalyzerProcessor.cpp
        source/analyzer/AnalyzerProcessor.h
)

target_link_libraries(spectrum-analyzer-processor PUBLIC tad-bits choc farbot FastFourier)
target_include_directories(spectrum-analyzer-processor PUBLIC source/analyzer)
add_compiler_warnings(spectrum-analyzer-processor)

set(plugin_name "Spectrum")

if (NOT EMSCRIPTEN)
    CPMAddPackage("gh:free-audio/clap#1.2.10")
    CPMAddPackage("gh:free-audio/clap-helpers#a380c978f429103e08afc1d17e9efbf3e3c96ee2")

    set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE)
    set(CLAP_WRAPPER_OUTPUT_NAME "${plugin_name}")
    CPMAddPackage("gh:free-audio/clap-wrapper@0.17.0")
endif ()

CPMAddPackage("gh:VitalAudio/visage#238d1b542002e03ad42accd076f938304e67971d")
CPMAddPackage("gh:Neargye/magic_enum@0.9.8")
CPMAddPackage("gh:nlohmann/json@3.12.0")

file(GLOB_RECURSE font_files fonts/*.ttf)
add_embedded_resources(embedded-font-resources "Fonts.h" "resources::fonts" "${font_files}")

file(GLOB_RECURSE shader_files shaders/[vf]s_*.sc)
visage_embed_shaders(embedded-shader-resources "Shaders.h" "resources::shaders" "${shader_files}")

if (EMSCRIPTEN)
    # The plugin's UI & analyzer running in a web page, fed by the browser's audio input
    add_executable(spectrum-web source/web/WebMain.cpp)

    target_include_directories(spectrum-web PRIVATE source/plugin)
    target_link_libraries(spectrum-web PRIVATE
            spectrum-analyzer-processor
            visage embedded-font-resources embedded-shader-resources
            magic_enum nlohmann_json
    )

    target_compile_definitions(spectrum-web PRIVATE
            PLUGIN_NAME="${plugin_name}"
            PRODUCT_VERSION="${PROJECT_VERSION}"
            JSON_HAS_FILESYSTEM=0
            JSON_HAS_EXPERIMENTAL_FILESYSTEM=0
    )

    add_compiler_warnings(spectrum-web)

    # The EM_ASM macros trip these
    target_compile_options(spectrum-web PRIVATE -Wno-gnu-zero-variadic-macro-arguments -Wno-dollar-in-identifier-extension)

    target_link_options(spectrum-web PRIVATE
            -sENVIRONMENT=web
            -sALLOW_MEMORY_GROWTH
            -sGL_ENABLE_GET_PROC_ADDRESS
            "-sEXPORTED_FUNCTIONS=['_main','_pasteCallback']"
            "-sEXPORTED_RUNTIME_METHODS=['ccall','UTF8ToString','stringToNewUTF8','HEAPF32']"
    )

    # Everything that gets hosted ends up in <build dir>/web
    set(web_output_dir "${CMAKE_BINARY_DIR}/web")
    set_target_properties(spectrum-web PROPERTIES OUTPUT_NAME "spectrum" RUNTIME_OUTPUT_DIRECTORY "${web_output_dir}")

    file(GLOB web_files CONFIGURE_DEPENDS web/*)
    add_custom_command(TARGET spectrum-web POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${web_files} "${web_output_dir}"
            COMMENT "Copying web page files"
    )

    return()
endif ()

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

        # nlohmann/json's std::filesystem::path conversions need char8_t in C++20, but we build with -fno-char8_t
        JSON_HAS_FILESYSTEM=0
        JSON_HAS_EXPERIMENTAL_FILESYSTEM=0
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
    set(test_runner testrunner)
    add_executable(${test_runner} tests/AnalyzerProcessorTests.cpp)
    target_link_libraries(${test_runner} PRIVATE spectrum-analyzer-processor Catch2::Catch2WithMain)
    target_include_directories(${test_runner} PRIVATE source/analyzer)
    add_compiler_warnings(${test_runner})
endif ()