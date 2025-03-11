cmake_minimum_required(VERSION 3.15)

include(FetchContent)
include(cmake/CPM.cmake)

project(spectrum VERSION 0.0.0 LANGUAGES C CXX)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 14.0 CACHE STRING "Build for 14.0")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "CMAKE_BUILD_TYPE unspecified; picking Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)

if(APPLE)
  option(USE_SANITIZER "Build and link with ASAN" FALSE)
endif()

option(COPY_AFTER_BUILD "Copy the clap to ~/Library on macOS" TRUE)

CPMAddPackage("gh:free-audio/clap#1.2.6")
CPMAddPackage("gh:free-audio/clap-helpers#58ab81b1dc8219e859529c1306f364bb3aedf7d5")
CPMAddPackage("gh:tadmn/choc#7ea2feaa80cd498963d065cd0199a9ebd9c610a6")
CPMAddPackage("gh:tadmn/farbot#8807ed0c40c861018226bf764f3f61105344a719")
CPMAddPackage("gh:tadmn/FastFourier#1bb8d8d098e0b887a0a49578c5269acb92900973")
CPMAddPackage("gh:tadmn/visage#efcbca35a45cde70c1661200aac90a2a37cc19a7")

# visage embedded file resources
file(GLOB_RECURSE FONT_TTF_FILES fonts/*.ttf)
add_embedded_resources(EmbeddedFontResources "Fonts.h" "resources::fonts" "${FONT_TTF_FILES}")

file(GLOB SOURCE_FILES src/*.cpp src/*.h)
add_library(${PROJECT_NAME} MODULE ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} clap clap-helpers choc farbot FastFourier visage EmbeddedFontResources)

if(APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    BUNDLE True
    BUNDLE_EXTENSION clap
    MACOSX_BUNDLE_GUI_IDENTIFIER com.tadmn.${PROJECT_NAME}
    MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
    MACOSX_BUNDLE_BUNDLE_VERSION "0.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.0"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}.plist.in
  )

  target_compile_definitions(${PROJECT_NAME} PRIVATE IS_MAC=1)
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Werror
    $<$<BOOL:${USE_SANITIZER}>:-fsanitize=address>
    $<$<BOOL:${USE_SANITIZER}>:-fsanitize=undefined>
  )
  target_link_options(${PROJECT_NAME} PRIVATE
    $<$<BOOL:${USE_SANITIZER}>:-fsanitize=address>
    $<$<BOOL:${USE_SANITIZER}>:-fsanitize=undefined>
  )

  if(${COPY_AFTER_BUILD})
    message(STATUS "Will copy plugin after every build")
    set(products_folder ${CMAKE_BINARY_DIR})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${PROJECT_NAME}.clap to ~/Library/Audio/Plug-Ins/CLAP/"
      COMMAND ${CMAKE_COMMAND} -E make_directory "~/Library/Audio/Plug-Ins/CLAP"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${products_folder}/${PROJECT_NAME}.clap" "~/Library/Audio/Plug-Ins/CLAP/${PROJECT_NAME}.clap"
    )
  endif()
endif()

if((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wshadow-all
    -Wshorten-64-to-32
    -Wstrict-aliasing
    -Wuninitialized
    -Wunused-parameter
    -Wconversion
    -Wsign-compare
    -Wint-conversion
    -Wconditional-uninitialized
    -Wconstant-conversion
    -Wsign-conversion
    -Wbool-conversion
    -Wextra-semi
    -Wunreachable-code
    -Wcast-align
    -Wshift-sign-overflow
    -Wmissing-prototypes
    -Wnullable-to-nonnull-conversion
    -Wno-ignored-qualifiers
    -Wswitch-enum
    -Wpedantic
    -Wdeprecated
    -Wfloat-equal
    -Wmissing-field-initializers
    $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJCXX>>:
    -Wzero-as-null-pointer-constant
    -Wunused-private-field
    -Woverloaded-virtual
    -Wreorder
    -Winconsistent-missing-destructor-override>
    $<$<OR:$<COMPILE_LANGUAGE:OBJC>,$<COMPILE_LANGUAGE:OBJCXX>>:
    -Wunguarded-availability
    -Wunguarded-availability-new>)
endif()