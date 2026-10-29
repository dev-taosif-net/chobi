# SPDX-License-Identifier: GPL-3.0-or-later

# Toolkit-free logic: no Qt, no libmpv. Unit-tested on its own.
add_library(chobi_core STATIC
  core/KeyMap.cpp
  core/Language.cpp
  core/MediaFormats.cpp
  core/PlaybackController.cpp
  core/ResumeStore.cpp
  core/SubtitleFinder.cpp
  core/TimeFormat.cpp
  core/TrackLabel.cpp
)
target_include_directories(chobi_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(chobi_core PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

# Engine (libmpv) and Qt Widgets UI.
add_library(chobi_app STATIC
  engine/PlayerEngine.h
  engine/MpvEngine.cpp
  engine/MpvVideoWidget.cpp
  app/Bench.cpp
  app/ControlsBar.cpp
  app/MainWindow.cpp
  app/MediaPath.cpp
  app/MediaProbe.cpp
  app/ResumeService.cpp
  app/SeekSlider.cpp
  app/Settings.cpp
  app/SettingsDialog.cpp
)
target_link_libraries(chobi_app PUBLIC
  chobi_core Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Libmpv::Libmpv)
target_compile_definitions(chobi_app PUBLIC CHOBI_VERSION="${PROJECT_VERSION}")

qt_add_executable(chobi WIN32 main.cpp)
target_link_libraries(chobi PRIVATE chobi_app)

include(GNUInstallDirs)
install(TARGETS chobi RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
