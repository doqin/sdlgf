# Copies the demo's runtime assets next to the executable, skipping Aseprite source files.
#
# Aseprite files (*.aseprite) are the artists' editable sources; the game only ever loads the
# exported .png/.tmx/.wav files, so copying them into the build tree (and from there into the
# portable release zip) is wasted space.
#
# Runs at build time so a rebuild picks up new artwork without reconfiguring:
#   cmake -DSRC=<assets dir> -DDST=<target assets dir> -P cmake/CopyAssets.cmake

if(NOT DEFINED SRC OR NOT DEFINED DST)
    message(FATAL_ERROR "CopyAssets.cmake needs -DSRC=<dir> and -DDST=<dir>")
endif()
if(NOT IS_DIRECTORY "${SRC}")
    message(FATAL_ERROR "CopyAssets.cmake: SRC is not a directory: ${SRC}")
endif()

file(COPY "${SRC}/" DESTINATION "${DST}" REGEX ".*\\.(ase|aseprite)$" EXCLUDE)

# file(COPY) never deletes, so drop sources left behind by earlier builds that used to copy
# everything.
file(GLOB_RECURSE _stale_aseprite "${DST}/*.ase" "${DST}/*.aseprite")
if(_stale_aseprite)
    file(REMOVE ${_stale_aseprite})
endif()
