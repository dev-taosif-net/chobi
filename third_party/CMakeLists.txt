# Vendored third-party code. Built as static libraries with the upstream sources unmodified.

add_library(chobi_monocypher STATIC
  monocypher/monocypher.c
  monocypher/monocypher-ed25519.c
)
target_include_directories(chobi_monocypher PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/monocypher)
set_target_properties(chobi_monocypher PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON
  AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(chobi_monocypher PRIVATE -w)  # upstream code, not ours to lint
endif()
