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

CPMAddPackage("gh:tadmn/tad-bits#4868305a180a8d33a5d79c13efe0c30ec72e29b5")
include(${tad-bits_SOURCE_DIR}/cmake/compile-options.cmake)

CPMAddPackage("gh:free-audio/clap#1.2.6")
CPMAddPackage("gh:free-audio/clap-helpers#58ab81b1dc8219e859529c1306f364bb3aedf7d5")

set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "Download plugin dependencies")
set(CLAP_WRAPPER_OUTPUT_NAME "${PRODUCT_NAME}" CACHE BOOL "ClapAsVST3 Wrapper name")
CPMAddPackage("gh:free-audio/clap-wrapper#a610e1a0821d54f2c64d1f13fd737ba2ccf314ab")

CPMAddPackage("gh:tadmn/choc#7ea2feaa80cd498963d065cd0199a9ebd9c610a6")
CPMAddPackage("gh:tadmn/farbot#8807ed0c40c861018226bf764f3f61105344a719")
CPMAddPackage("gh:tadmn/FastFourier#aadb30964063a216b3a01337541dd817dc7f3952")
CPMAddPackage("gh:tadmn/visage#80bbab0e983ccf74840b5099b506b5d1dc96e3d1")
CPMAddPackage("gh:Neargye/magic_enum@0.9.7")
CPMAddPackage("gh:nlohmann/json@3.11.3")

# Visage embedded file resources
file(GLOB_RECURSE FONT_FILES fonts/*.ttf)
add_embedded_resources(EmbeddedFontResources "Fonts.h" "resources::fonts" "${FONT_FILES}")

file(GLOB_RECURSE ICON_FILES icons/*.svg)
add_embedded_resources(EmbeddedIconResources "Icons.h" "resources::icons" "${ICON_FILES}")

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.h)

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

  PLUGIN_FORMATS CLAP

  ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_artifacts
)