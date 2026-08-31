/**
 * @file psnd.h
 * @brief Central constants for the psnd project.
 *
 * This header provides program-wide constants that should be used instead of
 * hardcoded strings throughout the codebase. Module-specific limits remain
 * in their respective headers.
 */

#ifndef PSND_H
#define PSND_H

/* Program identity */
#define PSND_NAME           "psnd"

/* Version. The build system defines this from project(VERSION ...) in the
 * top-level CMakeLists.txt, which is the single source of truth. The literal
 * below is only a fallback for compiling these sources outside CMake; keep it
 * in sync with the project() version. */
#ifndef PSND_VERSION
#define PSND_VERSION        "0.2.1"
#endif

/* Configuration */
#define PSND_CONFIG_DIR     ".psnd"

/* Default MIDI port name for virtual ports */
#define PSND_MIDI_PORT_NAME "PSND_MIDI"

#endif /* PSND_H */
