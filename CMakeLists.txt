cmake_minimum_required(VERSION 3.28)

include(cmake/CPM.cmake)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "Build for 10.15")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(spectrum VERSION 0.0.0 LANGUAGES C CXX)
set(PRODUCT_NAME "Spectrum")

option(USE_SANITIZER "Build and link with ASAN" FALSE)
option(COPY_AFTER_BUILD "Will copy after build" TRUE)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "CMAKE_BUILD_TYPE unspecified; picking Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

CPMAddPackage("gh:tadmn/tad-bits#dead88c4c2b2f7cd92182170a2389b64e3fe6e3a")
include(${tad-bits_SOURCE_DIR}/cmake/compile-options.cmake)

CPMAddPackage("gh:free-audio/clap#1.2.6")
CPMAddPackage("gh:free-audio/clap-helpers#58ab81b1dc8219e859529c1306f364bb3aedf7d5")

set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "Download plugin dependencies")
set(CLAP_WRAPPER_OUTPUT_NAME "${PRODUCT_NAME}" CACHE BOOL "ClapAsVST3 Wrapper name")
CPMAddPackage("gh:free-audio/clap-wrapper#a610e1a0821d54f2c64d1f13fd737ba2ccf314ab")

CPMAddPackage("gh:tadmn/choc#7ea2feaa80cd498963d065cd0199a9ebd9c610a6")
CPMAddPackage("gh:tadmn/farbot#8807ed0c40c861018226bf764f3f61105344a719")
CPMAddPackage("gh:tadmn/FastFourier#c38992d0f898116380121beb068fdc39f2dbd540")
CPMAddPackage("gh:tadmn/visage#677b82d19747cb2331a5bc524836685e53f1f14c")
CPMAddPackage("gh:Neargye/magic_enum@0.9.7")
CPMAddPackage("gh:nlohmann/json@3.11.3")

# Visage embedded file resources
file(GLOB_RECURSE FONT_FILES fonts/*.ttf)
add_embedded_resources(EmbeddedFontResources "Fonts.h" "resources::fonts" "${FONT_FILES}")

file(GLOB_RECURSE ICON_FILES icons/*.svg)
add_embedded_resources(EmbeddedIconResources "Icons.h" "resources::icons" "${ICON_FILES}")

file(GLOB SOURCE_FILES src/*.cpp src/*.h)

add_library(${PROJECT_NAME}-impl STATIC ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME}-impl PUBLIC clap)
target_link_libraries(${PROJECT_NAME}-impl PRIVATE
  clap-helpers clap-wrapper-extensions
  tad-bits choc farbot FastFourier
  visage EmbeddedFontResources EmbeddedIconResources
  magic_enum nlohmann_json
)

target_compile_definitions(${PROJECT_NAME}-impl PRIVATE
  PRODUCT_NAME="${PRODUCT_NAME}"
  PRODUCT_VERSION="${PROJECT_VERSION}"
)

add_compiler_warnings(${PROJECT_NAME}-impl)

make_clapfirst_plugins(
  TARGET_NAME ${PROJECT_NAME}
  IMPL_TARGET ${PROJECT_NAME}-impl

  OUTPUT_NAME "${PRODUCT_NAME}"

  ENTRY_SOURCE src/SpectrumPluginEntry.cpp

  BUNDLE_IDENTIFER "com.tadmn.spectrum"
  BUNDLE_VERSION ${PROJECT_VERSION}

  COPY_AFTER_BUILD ${COPY_AFTER_BUILD}

  PLUGIN_FORMATS CLAP VST3 AUV2

  ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_assets

  # You can add a target-per-standalone you want. Syntax here is
  #   target-postfix output-name clap-id
  # This allows you to make multiple standalones from a multi-plugin clap
  STANDALONE_CONFIGURATIONS
  standalone "${PRODUCT_NAME}" "com.tadmn.spectrum"
)