# psnd_platform.cmake - Centralized platform detection and configuration
#
# This module provides reusable functions for platform-specific configuration,
# eliminating duplicate platform detection code across CMakeLists.txt files.
#
# Usage:
#   include(psnd_platform)
#   psnd_platform_link_audio(my_target)        # Link audio frameworks/libraries
#   psnd_platform_link_midi(my_target)         # Link MIDI frameworks/libraries
#   psnd_platform_add_pthread(my_target)       # Add pthread support
#   psnd_platform_add_posix_defs(my_target)    # Add POSIX definitions
#

# ==============================================================================
# Platform Detection
# ==============================================================================

# Cache platform detection results
if(NOT DEFINED PSND_PLATFORM_DETECTED)
    set(PSND_PLATFORM_DETECTED TRUE CACHE INTERNAL "Platform detection done")

    if(APPLE)
        set(PSND_PLATFORM_MACOS TRUE CACHE INTERNAL "Building on macOS")
        set(PSND_PLATFORM_UNIX TRUE CACHE INTERNAL "Building on Unix-like")
    elseif(UNIX AND NOT APPLE)
        set(PSND_PLATFORM_LINUX TRUE CACHE INTERNAL "Building on Linux")
        set(PSND_PLATFORM_UNIX TRUE CACHE INTERNAL "Building on Unix-like")

        # Find ALSA once at configuration time
        find_package(ALSA QUIET)
        if(ALSA_FOUND)
            set(PSND_ALSA_FOUND TRUE CACHE INTERNAL "ALSA found")
            set(PSND_ALSA_LIBRARIES "${ALSA_LIBRARIES}" CACHE INTERNAL "ALSA libraries")
            set(PSND_ALSA_INCLUDE_DIRS "${ALSA_INCLUDE_DIRS}" CACHE INTERNAL "ALSA includes")
        endif()
    elseif(WIN32)
        set(PSND_PLATFORM_WINDOWS TRUE CACHE INTERNAL "Building on Windows")
    endif()
endif()

# ==============================================================================
# Audio Framework/Library Linking
# ==============================================================================

# Link platform-specific audio frameworks/libraries to a target
#
# Usage: psnd_platform_link_audio(my_target [PUBLIC|PRIVATE])
#
function(psnd_platform_link_audio target)
    set(visibility PUBLIC)
    if(ARGC GREATER 1)
        set(visibility ${ARGV1})
    endif()

    if(PSND_PLATFORM_MACOS)
        target_link_libraries(${target} ${visibility}
            "-framework CoreAudio"
            "-framework AudioToolbox"
        )
    elseif(PSND_PLATFORM_LINUX)
        if(PSND_ALSA_FOUND)
            target_link_libraries(${target} ${visibility} ${PSND_ALSA_LIBRARIES})
            target_include_directories(${target} PRIVATE ${PSND_ALSA_INCLUDE_DIRS})
        endif()
    elseif(PSND_PLATFORM_WINDOWS)
        target_link_libraries(${target} ${visibility} winmm)
    endif()
endfunction()

# ==============================================================================
# MIDI Framework/Library Linking
# ==============================================================================

# Link platform-specific MIDI frameworks/libraries to a target
#
# Usage: psnd_platform_link_midi(my_target [PUBLIC|PRIVATE])
#
function(psnd_platform_link_midi target)
    set(visibility PUBLIC)
    if(ARGC GREATER 1)
        set(visibility ${ARGV1})
    endif()

    if(PSND_PLATFORM_MACOS)
        target_link_libraries(${target} ${visibility}
            "-framework CoreMIDI"
            "-framework CoreFoundation"
        )
    elseif(PSND_PLATFORM_LINUX)
        if(PSND_ALSA_FOUND)
            target_link_libraries(${target} ${visibility} ${PSND_ALSA_LIBRARIES})
            target_include_directories(${target} PRIVATE ${PSND_ALSA_INCLUDE_DIRS})
        endif()
    elseif(PSND_PLATFORM_WINDOWS)
        target_link_libraries(${target} ${visibility} winmm)
    endif()
endfunction()

# ==============================================================================
# Full Audio + MIDI Linking
# ==============================================================================

# Link all platform-specific audio and MIDI frameworks/libraries to a target
#
# Usage: psnd_platform_link_audio_midi(my_target [PUBLIC|PRIVATE])
#
function(psnd_platform_link_audio_midi target)
    set(visibility PUBLIC)
    if(ARGC GREATER 1)
        set(visibility ${ARGV1})
    endif()

    if(PSND_PLATFORM_MACOS)
        target_link_libraries(${target} ${visibility}
            "-framework CoreMIDI"
            "-framework CoreFoundation"
            "-framework CoreAudio"
            "-framework AudioToolbox"
        )
    elseif(PSND_PLATFORM_LINUX)
        target_compile_definitions(${target} PRIVATE _GNU_SOURCE)
        if(PSND_ALSA_FOUND)
            target_link_libraries(${target} ${visibility} ${PSND_ALSA_LIBRARIES})
            target_include_directories(${target} PRIVATE ${PSND_ALSA_INCLUDE_DIRS})
        endif()
        target_link_libraries(${target} ${visibility} pthread dl m)
    elseif(PSND_PLATFORM_WINDOWS)
        # shell32: ShellExecuteA, used to open the browser for --web-open
        target_link_libraries(${target} ${visibility} winmm ws2_32 shell32)
    endif()
endfunction()

# ==============================================================================
# POSIX/Threading Support
# ==============================================================================

# Add POSIX definitions for a target
#
# Usage: psnd_platform_add_posix_defs(my_target)
#
function(psnd_platform_add_posix_defs target)
    if(PSND_PLATFORM_UNIX)
        target_compile_definitions(${target} PRIVATE _GNU_SOURCE)
    endif()
endfunction()

# Add threading support to a target
#
# Usage: psnd_platform_add_pthread(my_target [PUBLIC|PRIVATE])
#
# On Windows, this is a no-op since the compat/thread.h header uses
# Windows threading primitives (CRITICAL_SECTION, CreateThread).
#
function(psnd_platform_add_pthread target)
    set(visibility PUBLIC)
    if(ARGC GREATER 1)
        set(visibility ${ARGV1})
    endif()

    if(PSND_PLATFORM_UNIX)
        target_link_libraries(${target} ${visibility} pthread)
    endif()
    # Windows: no additional libraries needed - uses built-in threading
endfunction()

# Add common Unix libraries (dl, m) to a target
#
# Usage: psnd_platform_add_unix_libs(my_target [PUBLIC|PRIVATE])
#
function(psnd_platform_add_unix_libs target)
    set(visibility PUBLIC)
    if(ARGC GREATER 1)
        set(visibility ${ARGV1})
    endif()

    if(PSND_PLATFORM_UNIX)
        target_link_libraries(${target} ${visibility} dl m)
    endif()
endfunction()

# ==============================================================================
# Compiler Warnings
# ==============================================================================

# Add standard warning flags for a target
#
# Usage: psnd_platform_add_warnings(my_target)
#
function(psnd_platform_add_warnings target)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# ==============================================================================
# Math Library
# ==============================================================================

# Add math library (libm) to a target
#
# Usage: psnd_platform_add_math(my_target [PUBLIC|PRIVATE])
#
function(psnd_platform_add_math target)
    set(visibility PUBLIC)
    if(ARGC GREATER 1)
        set(visibility ${ARGV1})
    endif()

    if(PSND_PLATFORM_UNIX)
        target_link_libraries(${target} ${visibility} m)
    endif()
endfunction()
