# Psnd TODO

## Known Bugs

### `:plugin presets` buffer switch not working

**Status:** Resolved (2026-01-31)

**Root cause:** After `buffer_switch()` in `command/plugin.c`, the local `ctx` pointer was stale - it still pointed to the old buffer's context. Subsequent operations like `editor_set_status_msg(ctx, ...)` were modifying the wrong buffer.

**Fix:** Refresh the context pointer after buffer switch:
```c
buffer_switch(buf_id);
ctx = buffer_get_current();  /* Refresh stale pointer */
```

**Original description:** The `:plugin presets` command was intended to open a new scratch buffer displaying all plugin presets. The command executed without errors, but the new buffer did not appear - the editor stayed on the original buffer.

---

## High Priority

### Cross-Platform Support

- [ ] Windows support (in progress)
  - [x] Terminal abstraction layer (`terminal.h`, `terminal_posix.c`, `terminal_win.c`)
    - Windows Console API with VT mode (Windows 10+) and legacy fallback
    - Alternate screen buffer, raw mode, resize detection
  - [x] Platform compatibility header (`compat/platform.h`)
    - Cross-platform `unistd.h` equivalents: `access()`, `isatty()`, `usleep()`, etc.
    - Path handling utilities
  - [x] Threading abstraction (`compat/thread.h`)
    - `psnd_mutex_t` with Windows CRITICAL_SECTION / POSIX pthread_mutex
    - Thread creation, condition variables
  - [x] CMake Windows support (`psnd_platform.cmake`)
    - Windows audio/MIDI linking (winmm, ws2_32)
  - [ ] Test on Windows
    - Requires Windows build environment (MSVC or MinGW)
    - Test terminal VT mode detection
    - Test audio/MIDI backends

- [ ] Build troubleshooting guide
  - Platform-specific dependency installation (macOS, Linux distros, Windows/MSYS2)
  - Common build errors and solutions
  - Audio/MIDI backend configuration per platform

- [ ] Web host as primary cross-platform UI
  - Already functional with xterm.js terminal emulator
  - [ ] Multiple client support (currently single WebSocket connection)
  - [ ] Session persistence (save/restore editor state across restarts)
  - [ ] Authentication (required before exposing to network)

### Stability & Robustness

- [x] ~~Add scanner/lexer unit tests for all languages~~ **DONE**
  - Alda scanner: 44 tests including vulnerability tests
  - Joy lexer: 59 tests including edge cases
  - Bog tokenizer: 41 tests including error recovery
  - TR7 reader: 61 tests including buffer boundary tests

- [x] ~~Refactor CLI tests to avoid shell spawning~~ **DONE**
  - Replaced `system()` with `fork`/`execve` via `test_exec()` in `test_process.h`
  - Uses `mkdtemp()` and `nftw()` for temp directory management

- [x] ~~Add missing test coverage~~ **DONE**
  - Added `test_lang_bridge.c`: 38 tests for language bridge dispatch
  - Extended `test_link.c`: 10 new tests for callbacks and tempo clamping
  - Added `test_repl_commands.c`: 52 tests for shared REPL command processor

- [x] ~~Expand test framework~~ **DONE**
  - Comparison macros: `ASSERT_GT`, `ASSERT_LT`, `ASSERT_GTE`, `ASSERT_LTE`
  - Fixture support: `FIXTURE`, `TEST_F`, `SUITE_SETUP`, `SUITE_TEARDOWN`
  - Memory leak detection: `test_memcheck.h` with allocation tracking

- [ ] Add fuzzing infrastructure (optional)
  - Consider libFuzzer or AFL++ for parser/scanner testing
  - Low priority unless targeting wider distribution

### Code Coverage

Current state: **72 test files**, **~2,390 test functions**, **~5,800 assertions**. Direct file mapping coverage ~70%.

**Critical gaps (no unit tests):**

- [x] ~~`main.c` / `lang_dispatch.c` - Entry point and mode selection~~ **DONE** (70 tests: 44 unit + 26 integration)
- [x] ~~`midi.c` / `midi_input.c` - Core MIDI subsystem~~ **DONE** (68 tests)
- [x] ~~`music_theory.c` - Chord construction, scale handling~~ **DONE** (73 tests)
- [x] ~~Tracker module (11 files)~~ **DONE** (654 tests)
  - [x] ~~tracker_model.c~~ **DONE** (70 tests)
  - [x] ~~tracker_engine.c~~ **DONE** (104 tests)
  - [x] ~~tracker_plugin.c~~ **DONE** (65 tests)
  - [x] ~~tracker_plugin_notes.c~~ **DONE** (included in plugin tests)
  - [x] ~~tracker_audio.c~~ **DONE** (included in engine tests)
  - [x] ~~tracker_view_json.c~~ **DONE** (77 tests)
  - [x] ~~tracker_view_undo.c~~ **DONE** (47 tests)
  - [x] ~~tracker_view_theme.c~~ **DONE** (49 tests)
  - [x] ~~tracker_view_clipboard.c~~ **DONE** (77 tests)
  - [x] ~~tracker_view.c~~ **DONE** (121 tests)
  - [x] ~~tracker_view_terminal.c~~ **DONE** (44 tests)
- [x] ~~Command system (11 files in `loki/command/`)~~ **DONE** (51 tests)
  - All handlers tested: goto, substitute, basic, file, export, metronome, link, loop, csd, theme, plugin

**Secondary gaps:**

- [x] ~~`config.c`, `keybind.c`, `theme_toml.c` - Configuration loading~~ **DONE** (41 tests)
- [x] ~~`renderer.c` - Terminal rendering~~ **DONE** (50 tests)
- [x] ~~`async.c` / `shared_async.c` - Async scheduling~~ **DONE** (62 tests)
- [x] ~~`jsonrpc.c`, `osc.c` - Protocol handlers~~ **DONE** (67 tests: 42 jsonrpc + 25 osc)

**Remaining test coverage work:**

- [ ] `lua.c` - Lua integration (extend existing `test_lua_api.c`)
  - [ ] Lua state lifecycle (init, cleanup, error recovery)
  - [ ] API bindings coverage (loki.*, alda.*, joy.*, link.*)
  - [ ] Script loading and execution
  - [ ] Error handling and sandboxing
  - [ ] Callback registration

- [ ] MHS (MicroHaskell) - 19 files, initial tests added (47 tests)
  - [x] MIDI FFI (32 tests) - cents-to-bend, random, recording, channel validation
  - [x] Context state (15 tests) - null safety, port API, lifecycle
  - [ ] Lexer/tokenizer (MicroHs runtime - low priority)
  - [ ] Parser (MicroHs runtime - low priority)
  - [ ] Type checker (MicroHs runtime - low priority)
  - [ ] Interpreter/evaluator (MicroHs runtime - low priority)

**Well-tested areas (for reference):**

| Module | Tests | Notes |
|--------|-------|-------|
| Alda parser/scanner | 169 | Strong core parsing coverage |
| Joy parser/primitives | 106 | Core language well tested |
| Bog language | 187 | Tokenizer, parser, builtins |
| TR7 reader/music | 102 | Good coverage |
| Loki editor core | 365 | Modal editing, undo, search |
| Shared backends | 168 | TSF, Csound, FluidSynth, Minihost |
| Music theory | 73 | Pitch, chords, scales, microtonal |
| Command handlers | 51 | All 11 command files covered |
| MIDI I/O | 68 | Context, ports, callbacks, timing |
| Async playback | 62 | Schedule, events, tick/ms modes, lifecycle |
| Config system | 41 | TOML parsing, keybindings, themes |
| Tracker model | 70 | Events, phrases, cells, tracks, patterns, songs |
| Tracker engine | 104 | Lifecycle, timing, transport, event queue, sync |
| Tracker plugin | 65 | Registry, compilation, evaluation, context, RNG |
| Tracker view | 415 | JSON, undo, theme, clipboard, core, terminal |
| MHS (MicroHaskell) | 47 | MIDI FFI, context state, port API |

### Code Quality (Completed)

- [x] ~~Extract shared REPL loop skeleton~~ **DONE**
- [x] ~~Centralize platform CMake logic~~ **DONE**
- [x] ~~Complete command dispatcher for all keybindings~~ **DONE**
- [x] ~~Extract buffer manager to injectable service~~ **DONE**
- [x] ~~Expand editor highlight vocabulary for full tree-sitter support~~ **DONE**

---

## Medium Priority

### Documentation

- [ ] API reference generation
  - Consider Doxygen for generated docs
  - Focus on public APIs: Lua bindings, language bridge, shared backend

- [ ] Contributing guide
  - Code style, PR process, test requirements

### Editor Features

- [x] ~~Playback visualization~~ **DONE**
  - [x] ~~Highlight currently playing region~~ **DONE**
    - [x] ~~Source line tracking in Alda events~~ **DONE** (`ALDA_SOURCE_TRACKING`)
    - [x] ~~Source line tracking in shared async~~ **DONE** (`SHARED_SOURCE_TRACKING`)
    - [x] ~~Joy parser source line tracking~~ **DONE**
    - [x] ~~TR7 async API source tracking~~ **DONE** (API ready, primitives use 0 - TR7 interpreter doesn't expose source positions)
    - [x] ~~Bog parser source line tracking~~ **DONE**
      - Tokenizer tracks line numbers per token
      - `BogClause.source_line` stores clause definition line
      - `BogSolutions.source_lines` tracks matched clause per solution
      - `bog_scheduler_get_current_source_line()` API for querying
      - `LokiLangOps.get_source_line` callback for language-agnostic query
    - [x] ~~Editor integration to highlight lines during playback~~ **DONE**
      - Line gutter shows `>` indicator in bright green for playing line
      - Row background highlighted with dark green during playback
      - `EditorView.playing_line` updated each frame from `shared_async_get_current_source_line()`
  - [x] ~~Show playback progress in status bar~~ **DONE**
    - Status bar shows bar.beat, tempo, Link peers, play/metronome state

- [x] ~~Tempo tap / Metronome toggle~~ **DONE**
  - `:tap` command for tap tempo (averages intervals, sets BPM via Link or context)
  - `:metronome` command with subdivisions (1=quarter, 2=eighth, 4=sixteenth)
  - Beat-synced via Link, uses drum sounds (kick/hi-hat)

- [ ] Split windows
  - Already designed for in `editor_ctx_t`
  - Requires screen rendering changes

### Ableton Link Integration

- [x] ~~Full transport sync~~ **DONE**
  - [x] ~~Wire transport callbacks to start/stop playback~~ **DONE**
    - `:link transport [on|off]` command to enable optional transport sync mode
    - When enabled, Link transport start plays the buffer, stop halts playback
    - Works independently of basic Link tempo sync
  - [x] ~~"Armed for playback" state~~ **DONE**
    - Status bar shows `[ARMED]` when transport sync enabled but waiting for Link start
    - `StatusInfo.transport_armed` field for renderer

---

## Low Priority

### Future Architecture

- [ ] Wrap editor core in standalone service process
  - Small RPC protocol (stdio JSON or gRPC)
  - Would enable embedding editor in other applications

- [ ] Lua-to-language primitive callbacks for TR7/Bog
  - Joy already implemented (`loki.joy.register_primitive`)
  - TR7 (Scheme): Moderate complexity
  - Bog (Prolog): High complexity (unification, backtracking)

- [ ] Plugin architecture for language modules
  - Dynamic loading of language support

- [ ] JACK backend
  - For pro audio workflows on Linux

### Editor Features

- [ ] LSP client integration
  - Would provide IDE-like features
  - High complexity undertaking

- [ ] Git integration
  - Gutter diff markers
  - Stage/commit commands

---

## Backlog: New Languages

- [ ] bytebeat (see: <https://dollchan.net/bytebeat>)
- [ ] funcbeat
- [ ] drumbeat (see: <https://wavepot.com>)

---

## Backlog: Tracker Enhancements

Future features for the tracker sequencer (`tracker_demo`):

- [ ] Automation lanes (per-row parameter automation)
- [ ] Live jam mode (real-time pattern triggering)
- [ ] Quantize (snap MIDI notes to grid)
- [ ] Piano roll view (graphical note editing)
- [ ] Pattern templates (save/load for reuse)
- [ ] Sample trigger (one-shot audio samples from cells)

---

## Backlog: Feature Opportunities

- [ ] Preset browser & layering UI
- [ ] Session capture & arrangement (MIDI timeline)
- [ ] Controller & automation mapping (MIDI CC, OSC)
- [ ] Cross-language patch sharing (messaging bus)
- [ ] Real-time visualization (playback state via OSC/WebSocket)
