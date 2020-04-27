# Convery (tcn@ayria.se)
cmake_minimum_required(VERSION 3.1)
project(AYRIA)

# Use the latest standard at this time.
set(CMAKE_CXX_STANDARD 20)
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
    if(CMAKE_BUILD_TYPE MATCHES Release)
        set(EXTRA_CMPFLAGS ${EXTRA_CMPFLAGS} "/permissive- /Gy /Gw /GR- /GL /Os /Oy /O2 /GF /Brepro")
        set(EXTRA_LNKFLAGS ${EXTRA_LNKFLAGS} "/LTCG /filealign:512 /opt:ref,icf,lbr /incremental:no /Brepro /cgthreads:8")
    else()
        set(EXTRA_CMPFLAGS ${EXTRA_CMPFLAGS} "/permissive- /Gy") # /d2cgsummary
        set(EXTRA_LNKFLAGS ${EXTRA_LNKFLAGS} "/time /debug:fastlink /cgthreads:8")
    endif()

    # Force static runtime.
    foreach(Flag
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
        string(REPLACE "/MD" "/MT" ${Flag} "${${Flag}}")
    endforeach()

    # Force warnings to level 4.
    if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
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

# If we have VCPKG installed, add the directories based on a common package.
find_package(OPENSSL)
if(OPENSSL_FOUND)
    find_path(VCPKG_INC openssl/aes.h)
    find_library(VCPKG_LIB_RAW libssl.lib)
    get_filename_component(VCPKG_LIB ${VCPKG_LIB_RAW} DIRECTORY)

    include_directories(${VCPKG_INC})
    link_directories(${VCPKG_LIB})
endif()

# Global utilities.
include_directories("${PROJECT_SOURCE_DIR}")
set(MODULE_LIBS ${MODULE_LIBS} Utilities)
add_subdirectory(Utilities)

# Add the sub-projects.
add_subdirectory(Ayria)
add_subdirectory(Injector)
add_subdirectory(Localnetworking)
add_subdirectory(Platformwrapper)

# Examples.
add_subdirectory(Plugintemplate)
