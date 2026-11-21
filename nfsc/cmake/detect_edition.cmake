# Picks the game edition to build for (see editions.cmake) and prepares its files:
#   - NFSC_EDITION: v7, v11, ...
#   - config/selected/functions.toml: copy of config/${NFSC_EDITION}/functions.toml, included by
#     nfsc_manifest.toml (not tracked by git)
#   - ${NFSC_GENERATED_INCLUDE_DIR}/nfsc_edition.h: NFSC_GAME_<EDITION> and the edition table
#
# NFSC_GAME_VERSION = auto reads it from game/default.xex. A forced edition still has to match
# game/default.xex when that exists: code recompiled from one edition must never get another
# edition's addresses.

set(_nfsc_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")
include("${_nfsc_cmake_dir}/editions.cmake")
include("${_nfsc_cmake_dir}/edition_select.cmake")
nfsc_check_editions()
list(JOIN NFSC_EDITIONS ", " _nfsc_list)

set(NFSC_GAME_VERSION auto CACHE STRING
    "Game edition to build for: auto (from game/default.xex) or one of ${_nfsc_list}")
set_property(CACHE NFSC_GAME_VERSION PROPERTY STRINGS auto ${NFSC_EDITIONS})

find_package(Python3 3.10 COMPONENTS Interpreter REQUIRED)

# The path in nfsc_manifest.toml ([entrypoint].file_path).
get_filename_component(_nfsc_xex "${CMAKE_CURRENT_SOURCE_DIR}/../game/default.xex" ABSOLUTE)
set(_nfsc_xexid "${CMAKE_CURRENT_SOURCE_DIR}/../tools/xexid.py")
if (EXISTS "${_nfsc_xex}")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_nfsc_xex}")
endif ()
nfsc_detect_edition("${_nfsc_xex}" "${Python3_EXECUTABLE}" "${_nfsc_xexid}" _nfsc_detected)
nfsc_choose_edition("${NFSC_GAME_VERSION}" "${_nfsc_detected}" "${_nfsc_xex}" NFSC_EDITION)
if (NOT _nfsc_detected)
  message(WARNING "nfsc: no game/default.xex: configuring only; building needs it")
endif ()
message(STATUS "nfsc: game edition ${NFSC_EDITION_${NFSC_EDITION}_NAME} (${NFSC_EDITION})")

# Function boundaries of this edition, at the path nfsc_manifest.toml includes.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config/${NFSC_EDITION}/functions.toml"
               "${CMAKE_CURRENT_SOURCE_DIR}/config/selected/functions.toml" COPYONLY)

# nfsc_edition.h
string(TOUPPER "${NFSC_EDITION}" NFSC_EDITION_UPPER)
set(NFSC_EDITION_TABLE "")
list(FIND NFSC_EDITIONS "${NFSC_EDITION}" NFSC_EDITION_INDEX)
foreach (_edition IN LISTS NFSC_EDITIONS)
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " _bytes "${NFSC_EDITION_${_edition}_DIGEST}")
  string(REGEX REPLACE ", $" "" _bytes "${_bytes}")
  string(APPEND NFSC_EDITION_TABLE
         "    {0x${NFSC_EDITION_${_edition}_VERSION}, {${_bytes}}, "
         "\"${NFSC_EDITION_${_edition}_NAME}\"},\n")
endforeach ()
set(NFSC_GENERATED_INCLUDE_DIR "${CMAKE_BINARY_DIR}/nfsc_generated")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/nfsc_edition.h.in"
               "${NFSC_GENERATED_INCLUDE_DIR}/nfsc_edition.h" @ONLY)

# Call after rexglue_project(): copies the toml again as a build step before codegen, for a
# default.xex replaced since the last configure (see select_functions.cmake).
function (nfsc_add_select_functions codegen_stamp)
  set(selected "${CMAKE_CURRENT_SOURCE_DIR}/config/selected/functions.toml")
  set(tomls "")
  foreach (edition IN LISTS NFSC_EDITIONS)
    list(APPEND tomls "${CMAKE_CURRENT_SOURCE_DIR}/config/${edition}/functions.toml")
  endforeach ()
  add_custom_command(
    OUTPUT "${selected}"
    COMMAND "${CMAKE_COMMAND}"
            "-DNFSC_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            "-DNFSC_XEX=${_nfsc_xex}"
            "-DNFSC_XEXID=${_nfsc_xexid}"
            "-DPython3_EXECUTABLE=${Python3_EXECUTABLE}"
            "-DNFSC_GAME_VERSION=${NFSC_GAME_VERSION}"
            "-DNFSC_DEPFILE=${CMAKE_CURRENT_BINARY_DIR}/select_functions.d"
            -P "${_nfsc_cmake_dir}/select_functions.cmake"
    DEPENDS ${tomls}
            "${_nfsc_cmake_dir}/editions.cmake"
            "${_nfsc_cmake_dir}/edition_select.cmake"
            "${_nfsc_cmake_dir}/select_functions.cmake"
            "${_nfsc_xexid}"
    DEPFILE "${CMAKE_CURRENT_BINARY_DIR}/select_functions.d"
    COMMENT "Selecting the function boundaries of the game edition"
    VERBATIM)
  add_custom_command(OUTPUT "${codegen_stamp}" APPEND DEPENDS "${selected}")
endfunction ()
