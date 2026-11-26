# Copies the demo's runtime assets next to the executable, skipping Aseprite source files.
#
# Aseprite files (*.aseprite) are the artists' editable sources; the game only ever loads the
# exported .png/.tmx/.ogg files, so copying them into the build tree (and from there into the
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

# file(COPY) never deletes, so mirror the source: drop anything in the destination that no longer
# exists in SRC (or is an Aseprite source). Otherwise a renamed or converted asset (e.g. a .wav
# replaced by an .ogg) lingers in the build tree and ends up in the release package.
file(GLOB_RECURSE _dst_files RELATIVE "${DST}" "${DST}/*")
foreach(_rel IN LISTS _dst_files)
    if(_rel MATCHES "\\.(ase|aseprite)$" OR NOT EXISTS "${SRC}/${_rel}")
        file(REMOVE "${DST}/${_rel}")
    endif()
endforeach()
