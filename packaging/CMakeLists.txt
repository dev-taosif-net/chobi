# SPDX-License-Identifier: GPL-3.0-or-later
if(UNIX AND NOT APPLE)
  include(GNUInstallDirs)
  set(CHOBI_APP_ID io.github.dev_taosif_net.Chobi)
  install(FILES linux/${CHOBI_APP_ID}.desktop
          DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES linux/${CHOBI_APP_ID}.svg
          DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
  install(FILES linux/${CHOBI_APP_ID}.metainfo.xml
          DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
endif()
