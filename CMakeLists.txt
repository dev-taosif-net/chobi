# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.21)

project(Chobi
  VERSION 0.2.0
  DESCRIPTION "Lightweight cross-platform video player"
  HOMEPAGE_URL "https://github.com/dev-taosif-net/chobi"
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(CHOBI_BUILD_TESTS "Build unit, integration and UI tests" ON)
option(CHOBI_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
# Distribution packages update through their package manager; they should turn this off.
option(CHOBI_ENABLE_UPDATER "Built-in update check and self-update (AppImage, Windows installer)" ON)

# Public half of the release signing key; see docs/releasing.md. Empty = updater inactive.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/packaging/update-signing-key.pub" CHOBI_UPDATE_PUBLIC_KEY)
string(STRIP "${CHOBI_UPDATE_PUBLIC_KEY}" CHOBI_UPDATE_PUBLIC_KEY)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall> $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
                      $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>)
  if(CHOBI_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
  endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Network)
find_package(Libmpv 2.3 REQUIRED)  # client API 2.3 = mpv 0.38 (loadfile index argument)

qt_standard_project_setup()

add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(packaging)

if(CHOBI_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
