# Build step, run with cmake -P before codegen (see detect_edition.cmake): copies
# config/<edition>/functions.toml of the edition of game/default.xex to config/selected/, the
# path nfsc_manifest.toml includes. Configure does the same, but after a new default.xex the
# build runs codegen before it reconfigures; without this step that codegen would read the
# previous edition's function boundaries.
#
#   -DNFSC_SOURCE_DIR=<nfsc>  -DNFSC_XEX=<game/default.xex>  -DNFSC_XEXID=<tools/xexid.py>
#   -DPython3_EXECUTABLE=<python>  -DNFSC_GAME_VERSION=<auto|edition>  -DNFSC_DEPFILE=<file>
#
# The copy is only written when it changes, so codegen does not rerun for nothing.

include("${CMAKE_CURRENT_LIST_DIR}/editions.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/edition_select.cmake")

set(_selected "${NFSC_SOURCE_DIR}/config/selected/functions.toml")
# default.xex goes in a depfile rather than DEPENDS: when it is missing, ninja runs this step
# (which says how to get it) instead of stopping with "no known rule to make it".
file(WRITE "${NFSC_DEPFILE}" "${_selected}: ${NFSC_XEX}\n")

nfsc_check_editions()
nfsc_detect_edition("${NFSC_XEX}" "${Python3_EXECUTABLE}" "${NFSC_XEXID}" _detected)
if (NOT _detected)
  nfsc_choose_edition(auto "" "${NFSC_XEX}" _edition)
endif ()
nfsc_choose_edition("${NFSC_GAME_VERSION}" "${_detected}" "${NFSC_XEX}" _edition)

file(COPY_FILE "${NFSC_SOURCE_DIR}/config/${_edition}/functions.toml" "${_selected}"
     ONLY_IF_DIFFERENT)
