# Convery (tcn@ayria.se)
cmake_minimum_required(VERSION 3.11)
project(AYRIA)

# Use the latest standard at this time.
set(CMAKE_CXX_STANDARD 23)
enable_language(ASM_MASM)

# Export to the a gitignored directory.
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Bin)

# Separate release and debug-builds.
mark_as_advanced(CMAKE_IMPORT_LIBRARY_SUFFIX CMAKE_DEBUG_POSTFIX)
set(CMAKE_DEBUG_POSTFIX "d")

# Per compiler instructions.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

    # Windows compilation.
    if(CMAKE_BUILD_TYPE MATCHES Release OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
        set(EXTRA_CMPFLAGS "${EXTRA_CMPFLAGS} /Zc:__cplusplus /permissive- /W4 /Oiysyb3 /GF /Gy /Gw /GR- /GL /utf-8 /Brepro /fp:except-")
        set(EXTRA_LNKFLAGS "${EXTRA_LNKFLAGS} /LTCG /filealign:512 /opt:ref,icf,lbr /incremental:no /Brepro /cgthreads:8")
    else()
        set(EXTRA_CMPFLAGS "${EXTRA_CMPFLAGS} /Zc:__cplusplus /permissive- /Gy /utf-8 /fp:except-") # /d2TPReport
        set(EXTRA_LNKFLAGS "${EXTRA_LNKFLAGS} /time /debug:full /cgthreads:8")
    endif()

    # Need to specify the arch for x86.
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set(EXTRA_CMPFLAGS "${EXTRA_CMPFLAGS} /arch:SSE2")
    endif()

    # Force static runtime, warnings, and override default flags.
    foreach(Flag
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
        string(REGEX REPLACE "/W[0-4]" "/W4" ${Flag} "${${Flag}}")
        string(REPLACE "/GR " "/GR- " ${Flag} "${${Flag}}")
        string(REPLACE "/Ob2" "/Ob3" ${Flag} "${${Flag}}")
        string(REPLACE "/MD" "/MT" ${Flag} "${${Flag}}")
    endforeach()
else()
    # Assume GNU-GCC/CLANG.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -g")

    # For some reason my CMAKE fails to differentiate between x86 and x64.
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set(EXTRA_CMPFLAGS ${EXTRA_CMPFLAGS} "-m32")
        set(EXTRA_LNKFLAGS ${EXTRA_LNKFLAGS} "-m32 -static-libgcc -static-libstdc++")
    else()
        set(EXTRA_CMPFLAGS ${EXTRA_CMPFLAGS} "-m64")
        set(EXTRA_LNKFLAGS ${EXTRA_LNKFLAGS} "-m64 -static-libgcc -static-libstdc++")
    endif()
endif()

# Try to add the lib directories based on a required package.
find_path(STD_INC openssl/aes.h)
find_library(STD_LIB_RAW sqlite3.lib)
get_filename_component(STD_LIB ${STD_LIB_RAW} DIRECTORY)
include_directories(${STD_INC})
link_directories(${STD_LIB})

# Extra packages.
include(FetchContent)
FetchContent_Declare(
    cpp-lazy
    GIT_REPOSITORY https://github.com/MarcDirven/cpp-lazy
    GIT_TAG "4.0.0"
    UPDATE_DISCONNECTED YES)
set(CPP-LAZY_USE_STANDALONE TRUE)
FetchContent_MakeAvailable(cpp-lazy)
include_directories("${cpp-lazy_SOURCE_DIR}/include")

# Global utilities.
include_directories("${PROJECT_SOURCE_DIR}")
set(MODULE_LIBS ${MODULE_LIBS} Utilities)
add_subdirectory(Utilities)

# Add the sub-projects.
add_subdirectory(Ayria)
#add_subdirectory(Injector)
#add_subdirectory(Localnetworking)
add_subdirectory(Platformwrapper)

## Examples.
#add_subdirectory(Plugintemplate)
