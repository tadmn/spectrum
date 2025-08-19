cmake_minimum_required(VERSION 3.28)

include(cmake/CPM.cmake)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # Use static runtimes on Windows
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "Build for 10.15")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(spectrum VERSION 1.2.0 LANGUAGES C CXX)

option(USE_SANITIZER "Build and link with the address sanitizer" OFF)
option(BUILD_TESTS "Build unit test executable" OFF)
option(BUILD_PLUGIN "Build audio plugin" OFF)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "CMAKE_BUILD_TYPE unspecified; picking Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

CPMAddPackage("gh:tadmn/tad-bits#500124abce002a69664b3324fba8bff24556f1c4")
include(${tad-bits_SOURCE_DIR}/cmake/compile-options.cmake)

CPMAddPackage("gh:tadmn/choc#2f5b8627708ff4702b4fb6646fddf0c5f4a52007")
CPMAddPackage("gh:tadmn/dsp#7378801601d9d1aae07923f7049d30f858cd447b")
CPMAddPackage("gh:tadmn/farbot#3cf08080cef9e7c0347fdc144e81b02f5e27a8ca")
CPMAddPackage("gh:tadmn/FastFourier#453106d9d2995aaca8cf53ea556749c1e07bdb05")

add_library(spectrum-analyzer-processor STATIC src/analyzer/AnalyzerProcessor.cpp src/analyzer/AnalyzerProcessor.h)
target_link_libraries(spectrum-analyzer-processor PUBLIC tad-bits choc dsp farbot FastFourier)
target_include_directories(spectrum-analyzer-processor PUBLIC src/analyzer)
add_compiler_warnings(spectrum-analyzer-processor)

if (BUILD_TESTS)
  CPMAddPackage("gh:catchorg/Catch2@3.8.1")
  set(TEST_RUNNER testrunner)
  add_executable(${TEST_RUNNER} tests/AnalyzerProcessorTests.cpp)
  target_link_libraries(${TEST_RUNNER} PRIVATE spectrum-analyzer-processor Catch2::Catch2WithMain)
  target_include_directories(${TEST_RUNNER} PRIVATE src)
  add_compiler_warnings(${TEST_RUNNER})
endif()

if (BUILD_PLUGIN)
  set(PLUGIN_NAME "Spectrum")

  CPMAddPackage("gh:free-audio/clap#1.2.6")
  CPMAddPackage("gh:free-audio/clap-helpers#58ab81b1dc8219e859529c1306f364bb3aedf7d5")

  set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "Download plugin dependencies")
  set(CLAP_WRAPPER_OUTPUT_NAME "${PLUGIN_NAME}" CACHE BOOL "ClapAsVST3 Wrapper name")
  CPMAddPackage("gh:free-audio/clap-wrapper#5ba58f16a7816085d46cb8b7bb40b1eb1d9561d2")

  CPMAddPackage("gh:tadmn/visage#31a6e393dfe62d0c5973a02641bd44e78f1a2485")
  CPMAddPackage("gh:Neargye/magic_enum@0.9.7")
  CPMAddPackage("gh:nlohmann/json@3.11.3")

  # Visage embedded file resources
  file(GLOB_RECURSE FONT_FILES fonts/*.ttf)
  add_embedded_resources(EmbeddedFontResources "Fonts.h" "resources::fonts" "${FONT_FILES}")

  file(GLOB_RECURSE ICON_FILES icons/*.svg)
  add_embedded_resources(EmbeddedIconResources "Icons.h" "resources::icons" "${ICON_FILES}")

  file(GLOB_RECURSE SOURCE_FILES src/plugin/*.cpp src/plugin/*.h)
  add_library(${PROJECT_NAME}-impl STATIC ${SOURCE_FILES})

  target_link_libraries(${PROJECT_NAME}-impl PUBLIC clap)
  target_link_libraries(${PROJECT_NAME}-impl PRIVATE
    spectrum-analyzer-processor
    clap-helpers clap-wrapper-extensions
    visage EmbeddedFontResources EmbeddedIconResources
    magic_enum nlohmann_json
  )

  target_compile_definitions(${PROJECT_NAME}-impl PRIVATE
    PLUGIN_NAME="${PLUGIN_NAME}"
    PRODUCT_VERSION="${PROJECT_VERSION}"
  )

  add_compiler_warnings(${PROJECT_NAME}-impl)

  make_clapfirst_plugins(
    TARGET_NAME ${PROJECT_NAME}
    IMPL_TARGET ${PROJECT_NAME}-impl

    OUTPUT_NAME "${PLUGIN_NAME}"

    ENTRY_SOURCE src/plugin/SpectrumPluginEntry.cpp

    BUNDLE_IDENTIFIER "com.tadmn.spectrum"
    BUNDLE_VERSION ${PROJECT_VERSION}

    COPY_AFTER_BUILD ON

    PLUGIN_FORMATS CLAP

    ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_artifacts
  )
endif()