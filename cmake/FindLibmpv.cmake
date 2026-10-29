# SPDX-License-Identifier: GPL-3.0-or-later
#
# Finds libmpv and defines the imported target Libmpv::Libmpv.
#
# pkg-config is only used as a hint: on MSYS2 it reports MSYS-style paths ("/ucrt64/include")
# that native CMake cannot use, so the actual header and library are located with
# find_path/find_library, which also works for MSVC-style SDKs (set Libmpv_ROOT).

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PC_LIBMPV QUIET mpv)
endif()

find_path(Libmpv_INCLUDE_DIR mpv/client.h HINTS ${PC_LIBMPV_INCLUDE_DIRS})
find_library(Libmpv_LIBRARY NAMES mpv libmpv mpv-2 HINTS ${PC_LIBMPV_LIBRARY_DIRS})

if(Libmpv_INCLUDE_DIR AND EXISTS "${Libmpv_INCLUDE_DIR}/mpv/client.h")
  file(STRINGS "${Libmpv_INCLUDE_DIR}/mpv/client.h" _mpv_version_line
       REGEX "^#define MPV_CLIENT_API_VERSION MPV_MAKE_VERSION\\([0-9]+, *[0-9]+\\)")
  if(_mpv_version_line MATCHES "MPV_MAKE_VERSION\\(([0-9]+), *([0-9]+)\\)")
    set(Libmpv_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}")  # client API version
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libmpv
  REQUIRED_VARS Libmpv_LIBRARY Libmpv_INCLUDE_DIR
  VERSION_VAR Libmpv_VERSION)

if(Libmpv_FOUND AND NOT TARGET Libmpv::Libmpv)
  add_library(Libmpv::Libmpv UNKNOWN IMPORTED)
  set_target_properties(Libmpv::Libmpv PROPERTIES
    IMPORTED_LOCATION "${Libmpv_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Libmpv_INCLUDE_DIR}")
endif()

mark_as_advanced(Libmpv_INCLUDE_DIR Libmpv_LIBRARY)
