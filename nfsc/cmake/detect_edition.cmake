# Picks the game edition to build for (see editions.cmake) and prepares its files:
#   - NFSC_EDITION: v7, v11, ...
#   - config/selected/functions.toml: copy of config/${NFSC_EDITION}/functions.toml, included by
#     nfsc_manifest.toml (not tracked by git)
#   - ${NFSC_GENERATED_INCLUDE_DIR}/nfsc_edition.h: NFSC_GAME_<EDITION> and the edition table
#
# NFSC_GAME_VERSION = auto reads it from game/default.xex. A forced edition still has to match
# game/default.xex when that exists: code recompiled from one edition must never get another
# edition's addresses.

include("${CMAKE_CURRENT_LIST_DIR}/editions.cmake")
list(JOIN NFSC_EDITIONS ", " _nfsc_list)

set(NFSC_GAME_VERSION auto CACHE STRING
    "Game edition to build for: auto (from game/default.xex) or one of ${_nfsc_list}")
set_property(CACHE NFSC_GAME_VERSION PROPERTY STRINGS auto ${NFSC_EDITIONS})
if (NOT NFSC_GAME_VERSION STREQUAL "auto" AND NOT NFSC_GAME_VERSION IN_LIST NFSC_EDITIONS)
  message(FATAL_ERROR "NFSC_GAME_VERSION must be auto or one of ${_nfsc_list} "
                      "(got '${NFSC_GAME_VERSION}')")
endif ()

set(_nfsc_names "")
foreach (_edition IN LISTS NFSC_EDITIONS)
  string(APPEND _nfsc_names "\n  ${_edition}: ${NFSC_EDITION_${_edition}_NAME}")
endforeach ()

find_package(Python3 3.10 COMPONENTS Interpreter REQUIRED)

# The path in nfsc_manifest.toml ([entrypoint].file_path).
get_filename_component(_nfsc_xex "${CMAKE_CURRENT_SOURCE_DIR}/../game/default.xex" ABSOLUTE)
set(_nfsc_detected "")
if (EXISTS "${_nfsc_xex}")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_nfsc_xex}")
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}/../tools/xexid.py"
            "${_nfsc_xex}"
    OUTPUT_VARIABLE _nfsc_identity
    ERROR_VARIABLE _nfsc_identity_error
    RESULT_VARIABLE _nfsc_result
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (NOT _nfsc_result EQUAL 0
      OR NOT _nfsc_identity MATCHES "^title=([0-9A-F]+) version=([0-9A-F]+) digest=([0-9a-f]+)$")
    message(FATAL_ERROR "nfsc: could not read ${_nfsc_xex}: ${_nfsc_identity_error}")
  endif ()
  set(_nfsc_title "${CMAKE_MATCH_1}")
  set(_nfsc_version "${CMAKE_MATCH_2}")
  set(_nfsc_digest "${CMAKE_MATCH_3}")
  foreach (_edition IN LISTS NFSC_EDITIONS)
    if (_nfsc_title STREQUAL NFSC_TITLE_ID
        AND _nfsc_version STREQUAL NFSC_EDITION_${_edition}_VERSION
        AND _nfsc_digest STREQUAL NFSC_EDITION_${_edition}_DIGEST)
      set(_nfsc_detected "${_edition}")
    endif ()
  endforeach ()
  if (NOT _nfsc_detected)
    message(FATAL_ERROR
            "nfsc: game/default.xex is not a supported edition (title ID ${_nfsc_title}, version "
            "${_nfsc_version}, first page digest ${_nfsc_digest}). Supported:${_nfsc_names}")
  endif ()
endif ()

if (NFSC_GAME_VERSION STREQUAL "auto")
  if (NOT _nfsc_detected)
    message(FATAL_ERROR
            "nfsc: ${_nfsc_xex} not found. Extract it from your disc image first:\n"
            "  python tools/xdvdfs.py extract \"path/to/your/disc.iso\" game default.xex\n"
            "or set NFSC_GAME_VERSION to one of ${_nfsc_list} to check only the host code.")
  endif ()
  set(NFSC_EDITION "${_nfsc_detected}")
else ()
  set(NFSC_EDITION "${NFSC_GAME_VERSION}")
  if (_nfsc_detected AND NOT _nfsc_detected STREQUAL NFSC_EDITION)
    message(FATAL_ERROR
            "nfsc: game/default.xex is ${NFSC_EDITION_${_nfsc_detected}_NAME}, but "
            "NFSC_GAME_VERSION is ${NFSC_EDITION} (${NFSC_EDITION_${NFSC_EDITION}_NAME}). Set "
            "NFSC_GAME_VERSION to auto or ${_nfsc_detected}.")
  elseif (NOT _nfsc_detected)
    message(WARNING "nfsc: no game/default.xex: only the host code can be checked")
  endif ()
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
