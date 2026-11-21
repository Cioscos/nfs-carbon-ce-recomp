# Choosing the game edition, shared by detect_edition.cmake (configure) and
# select_functions.cmake (build step). Include editions.cmake first.

# Sets <out> to TRUE if <value> is <length> characters, each matching <class> (CMake regular
# expressions have no {n}).
function (_nfsc_is_hex value class length out)
  string(LENGTH "${value}" actual)
  if (value MATCHES "^${class}+$" AND actual EQUAL length)
    set(${out} TRUE PARENT_SCOPE)
  else ()
    set(${out} FALSE PARENT_SCOPE)
  endif ()
endfunction ()

# Stops if an entry of editions.cmake is not in the form tools/xexid.py prints, which would
# otherwise never match and be reported as an unsupported game.
function (nfsc_check_editions)
  _nfsc_is_hex("${NFSC_TITLE_ID}" "[0-9A-F]" 8 ok)
  if (NOT ok)
    message(FATAL_ERROR "nfsc: editions.cmake: NFSC_TITLE_ID must be 8 uppercase hex digits "
                        "(got '${NFSC_TITLE_ID}')")
  endif ()
  foreach (edition IN LISTS NFSC_EDITIONS)
    _nfsc_is_hex("${NFSC_EDITION_${edition}_VERSION}" "[0-9A-F]" 8 ok)
    if (NOT ok)
      message(FATAL_ERROR "nfsc: editions.cmake: NFSC_EDITION_${edition}_VERSION must be 8 "
                          "uppercase hex digits (got '${NFSC_EDITION_${edition}_VERSION}')")
    endif ()
    _nfsc_is_hex("${NFSC_EDITION_${edition}_DIGEST}" "[0-9a-f]" 40 ok)
    if (NOT ok)
      message(FATAL_ERROR "nfsc: editions.cmake: NFSC_EDITION_${edition}_DIGEST must be 40 "
                          "lowercase hex digits (got '${NFSC_EDITION_${edition}_DIGEST}')")
    endif ()
    if (NOT NFSC_EDITION_${edition}_NAME)
      message(FATAL_ERROR "nfsc: editions.cmake: NFSC_EDITION_${edition}_NAME is empty")
    endif ()
  endforeach ()
endfunction ()

# Sets <out> to the edition of <xex>, or to "" if <xex> does not exist. Stops if it cannot be
# read or is not one of NFSC_EDITIONS.
function (nfsc_detect_edition xex python xexid out)
  if (NOT EXISTS "${xex}")
    set(${out} "" PARENT_SCOPE)
    return ()
  endif ()
  execute_process(
    COMMAND "${python}" -I "${xexid}" "${xex}"
    OUTPUT_VARIABLE identity
    ERROR_VARIABLE identity_error
    RESULT_VARIABLE result
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (NOT result EQUAL 0
      OR NOT identity MATCHES "^title=([0-9A-F]+) version=([0-9A-F]+) digest=([0-9a-f]+)$")
    message(FATAL_ERROR "nfsc: could not read ${xex}: ${identity_error}")
  endif ()
  set(title "${CMAKE_MATCH_1}")
  set(version "${CMAKE_MATCH_2}")
  set(digest "${CMAKE_MATCH_3}")
  set(names "")
  foreach (edition IN LISTS NFSC_EDITIONS)
    string(APPEND names "\n  ${edition}: ${NFSC_EDITION_${edition}_NAME}")
    if (title STREQUAL NFSC_TITLE_ID
        AND version STREQUAL NFSC_EDITION_${edition}_VERSION
        AND digest STREQUAL NFSC_EDITION_${edition}_DIGEST)
      set(${out} "${edition}" PARENT_SCOPE)
      return ()
    endif ()
  endforeach ()
  message(FATAL_ERROR
          "nfsc: game/default.xex is not a supported edition (title ID ${title}, version "
          "${version}, first page digest ${digest}). Supported:${names}")
endfunction ()

# Sets <out> to the edition to build for: <requested> (auto or one of NFSC_EDITIONS), checked
# against <detected> (from nfsc_detect_edition; "" when <xex> does not exist). A forced edition
# still has to match <xex> when that exists: code recompiled from one edition must never get
# another edition's addresses.
function (nfsc_choose_edition requested detected xex out)
  list(JOIN NFSC_EDITIONS ", " list)
  if (NOT requested STREQUAL "auto" AND NOT requested IN_LIST NFSC_EDITIONS)
    message(FATAL_ERROR "NFSC_GAME_VERSION must be auto or one of ${list} "
                        "(got '${requested}')")
  endif ()
  if (requested STREQUAL "auto")
    if (NOT detected)
      message(FATAL_ERROR
              "nfsc: ${xex} not found. Extract it from your disc image first. From the "
              "repository root:\n"
              "  python tools/xdvdfs.py extract \"path/to/your/disc.iso\" game default.xex\n"
              "or set NFSC_GAME_VERSION to one of ${list} to configure without it (building "
              "needs default.xex).")
    endif ()
    set(${out} "${detected}" PARENT_SCOPE)
    return ()
  endif ()
  if (detected AND NOT detected STREQUAL requested)
    message(FATAL_ERROR
            "nfsc: game/default.xex is ${NFSC_EDITION_${detected}_NAME}, but "
            "NFSC_GAME_VERSION is ${requested} (${NFSC_EDITION_${requested}_NAME}). Set "
            "NFSC_GAME_VERSION to auto or ${detected}.")
  endif ()
  set(${out} "${requested}" PARENT_SCOPE)
endfunction ()
