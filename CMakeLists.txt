# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.21)

project(Chobi
  VERSION 0.1.0
  DESCRIPTION "Lightweight cross-platform video player"
  HOMEPAGE_URL "https://github.com/dev-taosif-net/chobi"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(CHOBI_BUILD_TESTS "Build unit, integration and UI tests" ON)
option(CHOBI_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(CHOBI_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
  endif()
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv>=2.0)

qt_standard_project_setup()

add_subdirectory(src)
add_subdirectory(packaging)

if(CHOBI_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
