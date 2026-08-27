# Unused Code Report

This document catalogs unused functions, variables, and parameters that generate compiler warnings. These are kept intentionally for future use, conditional compilation, or work in progress.

## editor.c

| Line | Type | Name | Notes |
|------|------|------|-------|
| 66 | function | `osc_query_get_filename` | OSC query callback - used when OSC is enabled |
| 72 | function | `osc_query_get_position` | OSC query callback - used when OSC is enabled |
| 112 | function | `exec_lua_command` | Lua command execution helper |
| 214 | function | `lua_apply_highlight_row` | Lua-based syntax highlighting |
| 340 | variable | `plugin_log` | Plugin logging path (minihost backend) |

## modal.c

| Line | Type | Name | Notes |
|------|------|------|-------|
| 70 | function | `is_joy_file` | Joy file detection helper |

## jsonrpc.c

| Line | Type | Name | Notes |
|------|------|------|-------|
| 130 | function | `respond_ok_with` | JSON-RPC response helper with payload |

## langs/joy/register.c

| Line | Type | Name | Notes |
|------|------|------|-------|
| 67 | variable | `JOY_LUA_PRIMITIVES_KEY` | Lua registry key for Joy primitives |

## langs/mhs/repl.c

| Line | Type | Name | Notes |
|------|------|------|-------|
| 418 | function | `mhs_stop_playback` | Playback control (WIP) |
| 815 | function | `build_mhs_argv` | Argument building helper (WIP) |
| 816 | parameter | `path_buf2` | Secondary path buffer |
| 1038 | variable | `path_arg2` | Secondary path argument |
| 1210 | variable | `path_arg2` | Secondary path argument |

## tests/loki/test_picker.c

| Line | Type | Name | Notes |
|------|------|------|-------|
| 148 | cast | `void*` to `int` | Test framework callback pattern |

## Resolution Options

1. **Keep as-is**: Code is used conditionally or planned for future use

2. **Add `__attribute__((unused))`**: Silence warning while keeping code

3. **Guard with `#ifdef`**: For conditionally compiled features

4. **Remove**: If confirmed unused

## Third-Party Warnings (Not Addressable)

These warnings come from vendored dependencies in `source/thirdparty/`:

- **libuv**: VLA folded to constant array (3 warnings)

- **TinySoundFont**: Null pointer subtraction (40+ warnings)

- **miniaudio**: C23 extension attributes (3 warnings)

- **libremidi**: Typedef redefinition (1 warning)

- **midifile**: CMake deprecation warning

---
Generated: 2026-02-03
