cmake_minimum_required(VERSION 3.15)

include(FetchContent)

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

option(COPY_AFTER_BUILD "Copy the clap to ~/Library on MACOS" TRUE)

FetchContent_Declare(clap GIT_REPOSITORY https://github.com/free-audio/clap.git
  GIT_TAG main
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(clap_helpers GIT_REPOSITORY https://github.com/free-audio/clap-helpers.git
  GIT_TAG main
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(FastFourier GIT_REPOSITORY https://github.com/tadmn/FastFourier.git
  GIT_TAG master
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(visage GIT_REPOSITORY https://github.com/VitalAudio/visage.git
  GIT_TAG main
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL
)

# Bit of a hack for header-only libs with no cmake support.
# See https://stackoverflow.com/questions/79261625/cmake-fetchcontent-with-header-only-project.
FetchContent_Declare(choc GIT_REPOSITORY https://github.com/Tracktion/choc.git
  GIT_TAG main
  GIT_SHALLOW TRUE
  SOURCE_SUBDIR ""
)

FetchContent_Declare(farbot GIT_REPOSITORY https://github.com/tadmn/farbot.git
  GIT_TAG master
  GIT_SHALLOW TRUE
  SOURCE_SUBDIR ""
)

FetchContent_MakeAvailable(clap)
FetchContent_MakeAvailable(clap_helpers)
FetchContent_MakeAvailable(choc)
FetchContent_MakeAvailable(FastFourier)
FetchContent_MakeAvailable(farbot)
FetchContent_MakeAvailable(visage)

file(GLOB SOURCE_FILES src/*.cpp src/*.h)
add_library(${PROJECT_NAME} MODULE ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} clap clap-helpers FastFourier visage)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${FETCHCONTENT_BASE_DIR}/choc-src/audio")
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${FETCHCONTENT_BASE_DIR}/choc-src/platform")
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${FETCHCONTENT_BASE_DIR}/farbot-src/include")

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