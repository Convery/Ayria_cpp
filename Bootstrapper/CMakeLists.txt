cmake_minimum_required(VERSION 3.1)

# Get the modulename from the directory.
get_filename_component(Directory ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" Directory ${Directory})
set(MODULENAME ${Directory})

# Special case so we can differentiate between builds.
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    string(APPEND MODULENAME "64")
    else()
    string(APPEND MODULENAME "32")
endif()

# Platform libraries to be linked.
if(WIN32)
    set(PLATFORM_LIBS)
else()
    set(PLATFORM_LIBS dl pthread)
endif()

# Third-party libraries, prefer find_library() and find_path().
set(MODULE_LIBS ${MODULE_LIBS})

# Just pull all the files from /Source
file(GLOB_RECURSE SOURCES "Source/*.cpp")
file(GLOB_RECURSE ASSEMBLY "Source/*.asm")
add_definitions(-DMODULENAME="${MODULENAME}")
add_library(${MODULENAME} SHARED ${SOURCES} ${ASSEMBLY})
set_target_properties(${MODULENAME} PROPERTIES PREFIX "")
target_link_libraries(${MODULENAME} ${PLATFORM_LIBS} ${MODULE_LIBS})
set_target_properties(${MODULENAME} PROPERTIES COMPILE_FLAGS "${EXTRA_CMPFLAGS}" LINK_FLAGS "${EXTRA_LNKFLAGS}")
