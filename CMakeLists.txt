cmake_minimum_required(VERSION 3.24)

# Version comes from the VERSION file (single source of truth, see docs/versioning.md)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" GAMEHQ_VERSION_FULL)
string(STRIP "${GAMEHQ_VERSION_FULL}" GAMEHQ_VERSION_FULL)
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" GAMEHQ_VERSION_BASE "${GAMEHQ_VERSION_FULL}")

# The compact About / What's New modal must remain useful offline. Keep its
# bundled notes tied to VERSION at configure time so a release cannot quietly
# ship the previous build's notes.
set(GAMEHQ_RELEASE_NOTES_FILE "${CMAKE_CURRENT_SOURCE_DIR}/assets/release-notes.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${GAMEHQ_RELEASE_NOTES_FILE}")
file(READ "${GAMEHQ_RELEASE_NOTES_FILE}" GAMEHQ_RELEASE_NOTES_JSON)
string(JSON GAMEHQ_RELEASE_NOTES_VERSION ERROR_VARIABLE GAMEHQ_RELEASE_NOTES_ERROR
    GET "${GAMEHQ_RELEASE_NOTES_JSON}" version)
if(GAMEHQ_RELEASE_NOTES_ERROR OR NOT GAMEHQ_RELEASE_NOTES_VERSION STREQUAL GAMEHQ_VERSION_FULL)
    message(FATAL_ERROR
        "assets/release-notes.json version '${GAMEHQ_RELEASE_NOTES_VERSION}' "
        "must match VERSION '${GAMEHQ_VERSION_FULL}'")
endif()

project(GameHQ VERSION ${GAMEHQ_VERSION_BASE} LANGUAGES C CXX)

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable miniz examples" FORCE)
set(BUILD_FUZZERS OFF CACHE BOOL "Disable miniz fuzzers" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Disable miniz tests" FORCE)
set(INSTALL_PROJECT OFF CACHE BOOL "Disable miniz install rules" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static dependencies" FORCE)
FetchContent_Declare(miniz
    URL https://github.com/richgel999/miniz/archive/refs/tags/3.1.2.tar.gz
    URL_HASH SHA256=98468f8924934b723276680f85238b6c78bf1f8b49b4459cc9b7214a20e2e9fb
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(miniz)

# Brand.qml is the single source of truth for the visible product name. It is
# read at configure time so C++ and QML always expose the same branding.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/qml/Brand.qml" GAMEHQ_BRAND_QML)
string(REGEX MATCH "property string name: \"([^\"]+)\"" _brand_name_match "${GAMEHQ_BRAND_QML}")
set(GAMEHQ_BRAND_NAME "${CMAKE_MATCH_1}")
string(REGEX MATCH "property string slug: \"([^\"]+)\"" _brand_slug_match "${GAMEHQ_BRAND_QML}")
set(GAMEHQ_BRAND_SLUG "${CMAKE_MATCH_1}")
configure_file(src/config/Brand.h.in generated/Brand.h @ONLY)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)

if(WIN32)
    # MinGW's windres is not on PATH (project-local toolchain) — point CMake at
    # the one next to the C++ compiler before enabling the RC language.
    if(NOT CMAKE_RC_COMPILER)
        get_filename_component(_toolchain_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
        if(EXISTS "${_toolchain_bin}/windres.exe")
            # CACHE so the language-determination step (fresh configure) sees it.
            set(CMAKE_RC_COMPILER "${_toolchain_bin}/windres.exe"
                CACHE FILEPATH "windres from the project-local MinGW toolchain")
        endif()
    endif()
    enable_language(RC)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Sql Widgets Svg Multimedia Network)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

add_subdirectory(src)

# Tests are opt-in so a normal build/run stays as fast as it was; start.bat does
# not enable them. See docs/dev-setup.md.
option(GAMEHQ_BUILD_TESTS "Build the automated test target" OFF)
if(GAMEHQ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
