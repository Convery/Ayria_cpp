# Convery (tcn@ayria.se)
cmake_minimum_required(VERSION 3.1)
project(Ayria)

# Use the latest standard at this time.
set(CMAKE_CXX_STANDARD 17)
enable_language(ASM_MASM)

# Export to the a gitignored directory.
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Bin)

# Separate release and debugbuilds output and dependencies.
mark_as_advanced(CMAKE_IMPORT_LIBRARY_SUFFIX CMAKE_DEBUG_POSTFIX)
set(CMAKE_DEBUG_POSTFIX "d")

# For some reason my CMAKE fails to differentiate between x86 and x64.
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set(EXTRA_CMPFLAGS ${EXTRA_CMPFLAGS} "-m32")
        set(EXTRA_LNKFLAGS ${EXTRA_LNKFLAGS} "-m32 -static-libgcc -static-libstdc++")
    else()
        set(EXTRA_CMPFLAGS ${EXTRA_CMPFLAGS} "-m64")
        set(EXTRA_LNKFLAGS ${EXTRA_LNKFLAGS} "-m64 -static-libgcc -static-libstdc++")
    endif()
endif()

# Ensure that Windows versions has a normal main().
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    set(EXTRA_LNKFLAGS ${EXTRA_LNKFLAGS} "/subsystem:windows /entry:mainCRTStartup")
endif()

# Enable windows optimizations.
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC" AND CMAKE_BUILD_TYPE MATCHES RELEASE)
    set(EXTRA_CMPFLAGS ${EXTRA_CMPFLAGS} "/permissive- /Gy /Gw /GS- /GL /Os /Ox")
    set(EXTRA_LNKFLAGS ${EXTRA_LNKFLAGS} "/LTCG /safeseh:no /filealign:512 /opt:ref /incremental:no")
endif()

# Enforce static VC runtimes when building for Windows.
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    foreach(flag_var
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()

# Set warningflags per compiler.
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -g")
endif()

# Always include the root.
include_directories("${PROJECT_SOURCE_DIR}")

# Add the sub-projects.
#add_subdirectory(Core)
