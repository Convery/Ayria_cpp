cmake_minimum_required(VERSION 3.1)

# Get the modulename from the directory.
get_filename_component(Directory ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" Directory ${Directory})
set(MODULENAME ${Directory})
set(CMAKE_DEBUG_POSTFIX "")

# Platform libraries to be linked.
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(PLATFORM_LIBS dl pthread)
else()
    set(PLATFORM_LIBS)
endif()

# Just pull all the files from /Source
file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE ASSEMBLY "*.asm")
add_definitions(-DMODULENAME="${MODULENAME}")
add_library(${MODULENAME} STATIC ${SOURCES} ${ASSEMBLY})
set_target_properties(${MODULENAME} PROPERTIES PREFIX "")
target_link_libraries(${MODULENAME} ${PLATFORM_LIBS} ${MODULE_LIBS})
set_target_properties(${MODULENAME} PROPERTIES COMPILE_FLAGS "${EXTRA_CMPFLAGS}" LINK_FLAGS "${EXTRA_LNKFLAGS}")
