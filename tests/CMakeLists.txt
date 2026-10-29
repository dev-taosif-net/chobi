# SPDX-License-Identifier: GPL-3.0-or-later
find_package(Qt6 REQUIRED COMPONENTS Test)

set(CHOBI_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/fixtures")

function(chobi_add_test name)
  cmake_parse_arguments(T "" "" "SOURCES;LIBS;LABELS" ${ARGN})
  add_executable(${name} ${T_SOURCES})
  target_link_libraries(${name} PRIVATE ${T_LIBS} Qt6::Test)
  target_compile_definitions(${name} PRIVATE CHOBI_FIXTURE_DIR="${CHOBI_FIXTURE_DIR}")
  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES
    LABELS "${T_LABELS}"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

# Unit tests: core logic only (no Qt GUI, no libmpv).
foreach(t language subtitle_finder keymap playback_controller resume_store time_format track_label)
  chobi_add_test(test_${t} SOURCES unit/test_${t}.cpp LIBS chobi_core Qt6::Core LABELS unit)
endforeach()

# UI test: real MainWindow, fake engine, offscreen platform.
chobi_add_test(test_main_window SOURCES ui/test_main_window.cpp ui/FakeEngine.h
  LIBS chobi_app LABELS ui)

# Integration tests: real libmpv (headless) on generated media.
find_program(CHOBI_BASH bash)
find_program(CHOBI_FFMPEG ffmpeg)
if(CHOBI_BASH AND CHOBI_FFMPEG)
  add_test(NAME generate_fixtures
    COMMAND ${CHOBI_BASH} ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/generate.sh ${CHOBI_FIXTURE_DIR})
  set_tests_properties(generate_fixtures PROPERTIES FIXTURES_SETUP media LABELS integration)

  chobi_add_test(test_mpv_engine SOURCES integration/test_mpv_engine.cpp
    LIBS chobi_app LABELS integration)
  set_tests_properties(test_mpv_engine PROPERTIES FIXTURES_REQUIRED media TIMEOUT 300)
else()
  message(STATUS "bash/ffmpeg not found: integration tests disabled")
endif()
