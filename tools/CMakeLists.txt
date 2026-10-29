# SPDX-License-Identifier: GPL-3.0-or-later
# Release tooling (not installed). chobi-sign signs update manifests; see docs/releasing.md.
add_executable(chobi-sign chobi_sign.cpp)
target_link_libraries(chobi-sign PRIVATE chobi_monocypher)
set_target_properties(chobi-sign PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
