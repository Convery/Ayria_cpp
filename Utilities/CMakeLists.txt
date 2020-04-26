cmake_minimum_required(VERSION 3.1)

# Get the modulename from the directory.
get_filename_component(Directory ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" Directory ${Directory})
set(MODULENAME ${Directory})
set(CMAKE_DEBUG_POSTFIX "")

# Just pull all the files from /Source
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.c *.asm)
add_library(${MODULENAME} STATIC ${SOURCES})
add_definitions(-DMODULENAME="${MODULENAME}")
set_target_properties(${MODULENAME} PROPERTIES PREFIX "")
set_target_properties(${MODULENAME} PROPERTIES COMPILE_FLAGS "${EXTRA_CMPFLAGS}" LINK_FLAGS "${EXTRA_LNKFLAGS}")
