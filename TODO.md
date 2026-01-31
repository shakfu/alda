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

### Testing & Robustness

- [x] ~~Add scanner/lexer unit tests for all languages~~ **DONE**
  - Alda scanner: 44 tests including vulnerability tests
  - Joy lexer: 59 tests including edge cases
  - Bog tokenizer: 41 tests including error recovery
  - TR7 reader: 61 tests including buffer boundary tests

- [ ] Add fuzzing infrastructure
  - Parser robustness for malformed input
  - Consider AFL or libFuzzer integration
  - Target: scanners, parsers, MIDI import

- [x] ~~Refactor CLI tests to avoid shell spawning~~ **DONE**
  - Replaced `system()` with `fork`/`execve` via `test_exec()` in `test_process.h`
  - Uses `mkdtemp()` and `nftw()` for temp directory management
  - Added `test_process.h` with reusable process execution utilities

- [x] ~~Add missing test coverage~~ **DONE**
  - Added `test_lang_bridge.c`: 38 tests for language bridge dispatch (registration, lookup, lifecycle, eval, stop)
  - Extended `test_link.c`: 10 new tests for callbacks and tempo clamping (now 33 total Link tests)
  - Added `test_repl_commands.c`: 52 tests for shared REPL command processor (quit, stop, link, synth, csound, play, etc.)
  - Pointer/string comparisons can silently truncate in test framework (known limitation)

- [x] ~~Expand test framework with comparison macros~~ **DONE**
  - Added `ASSERT_GT`, `ASSERT_LT`, `ASSERT_GTE`, `ASSERT_LTE` to all test frameworks
- [x] ~~Add test fixture support (setup/teardown)~~ **DONE**
  - Added `FIXTURE`, `FIXTURE_SETUP`, `FIXTURE_TEARDOWN`, `TEST_F`, `RUN_TEST_F` macros
  - Added `SUITE_SETUP`, `SUITE_TEARDOWN` for suite-level fixtures
  - Added `BEGIN_TEST_SUITE_WITH_FIXTURE`, `END_TEST_SUITE_WITH_FIXTURE` macros
- [x] ~~Add memory leak detection hooks~~ **DONE**
  - Created `test_memcheck.h` with allocation tracking
  - `MEMCHECK_MALLOC`, `MEMCHECK_FREE`, `MEMCHECK_REALLOC`, `MEMCHECK_CALLOC`, `MEMCHECK_STRDUP`
  - `memcheck_begin()`, `memcheck_end()`, `memcheck_report()`, `ASSERT_NO_LEAKS()`
  - `BEGIN_TEST_SUITE_MEMCHECK`, `RUN_TEST_MEMCHECK` for per-test leak checking
  - 12 self-tests verifying leak detection functionality

### Code Quality & Refactoring

- [x] ~~Extract shared REPL loop skeleton~~ **DONE**
  - Created `ReplSkeletonConfig` struct with callback-based interface in `repl_helpers.h`
  - `repl_skeleton_run()` handles: interactive vs pipe mode detection, line editor init,
    tab completion, history loading/saving, raw mode, main loop, cleanup
  - Refactored Joy REPL to use skeleton as proof-of-concept
  - Other REPLs (Alda, Bog, TR7) can be similarly refactored

- [x] ~~Centralize platform CMake logic~~ **DONE**
  - Created `scripts/cmake/psnd_platform.cmake` module
  - Functions: `psnd_platform_link_audio_midi()`, `psnd_platform_add_warnings()`,
    `psnd_platform_add_math()`, `psnd_platform_add_pthread()`, etc.
  - Updated `source/core/CMakeLists.txt`, `source/langs/alda/CMakeLists.txt`,
    `source/langs/joy/CMakeLists.txt`, `source/langs/bog/CMakeLists.txt`

- [x] ~~Complete command dispatcher for all keybindings~~ **DONE**
  - Registered `eval_line`, `play_file`, and `quit` as keybind commands in `keybind.c`
  - Command handlers defined in `modal.c`: `cmd_eval_line()`, `cmd_play_file()`, `cmd_quit_keybind()`
  - Removed duplicate case handlers from `process_normal_mode()` and `process_insert_mode()`
  - All keybindings now configurable via TOML `[keybindings]` section

### Architecture

- [x] ~~Extract buffer manager to injectable service~~ **DONE**
  - Added `buffer_manager_t` struct with `*_in()` API variants
  - Global `g_buffer_manager` for backwards compatibility
  - Enables multi-editor and better testability

- [ ] Wrap editor core in standalone service process (optional)
  - Small RPC protocol (stdio JSON or gRPC)
  - Commands: load file, save, apply keystroke, get view state
  - Would enable embedding editor in other applications

---

## Medium Priority

### Documentation

- [ ] API reference generation
  - Consider Doxygen for generated docs

- [ ] Contributing guide

- [ ] Build troubleshooting
  - Platform-specific guidance

### Editor Improvements

- [ ] Split windows
  - Already designed for in `editor_ctx_t`
  - Requires screen rendering changes

- [ ] Playback visualization
  - Highlight currently playing region
  - Show playback progress in status bar

- [ ] Tempo tap
  - Tap key to set tempo

- [ ] Metronome toggle

### Web Host Enhancements

The web host is functional with xterm.js terminal emulator. Remaining work:

- [ ] Multiple client support
  - Currently supports single WebSocket connection
  - Add connection management for concurrent clients

- [ ] Session persistence
  - Save/restore editor state across server restarts
  - Optional auto-save of open buffers

- [ ] Authentication
  - Add basic auth or token-based access for remote access
  - Required before exposing to network

### Ableton Link Integration

- [ ] **Full Transport Sync** - Start/stop from any Link peer controls all
  - Wire transport callbacks to actually start/stop playback
  - Requires interruptible playback and a "armed for playback" state
  - Most complex - requires rethinking REPL interaction model

---

## Low Priority

### Platform Support

- [ ] Windows support
  - Editor uses POSIX headers: `termios.h`, `unistd.h`, `pthread.h`
  - Options: Native Windows console API, or web editor using CodeMirror/WebSockets

### Future Architecture

- [ ] Lua-to-language primitive callbacks for TR7/Bog
  - Joy already implemented (`loki.joy.register_primitive`)
  - TR7 (Scheme): Moderate complexity (value conversion, error handling)
  - Bog (Prolog): High complexity (unification, backtracking, term conversion)

- [ ] Plugin architecture for language modules
  - Dynamic loading of language support

- [ ] JACK backend
  - For pro audio workflows

- [ ] Provide a minimal language example

### Editor Features

- [x] ~~Expand editor highlight vocabulary for full tree-sitter support~~ **DONE**
  - 51 HL_* types now defined in `internal.h`
  - Full tree-sitter capture mapping in `treesitter.c`
  - TOML themes support all token types (function, operator, punctuation, variable.builtin, etc.)
  - See `.psnd/themes/*.toml` for complete color mappings

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

- [ ] **Automation Lanes**
  - Per-row parameter automation (volume, pan, CC values)
  - Interpolation modes: step, linear, exponential
  - Visual lane display alongside pattern grid
  - Automation curves for smooth transitions

- [ ] **Live Jam Mode**
  - Real-time performance with pattern triggering via keyboard
  - Instant scene/pattern switching without stopping playback
  - Keyboard-as-instrument mode for live note input
  - Quick mute/solo toggles with visual feedback

- [ ] **Quantize**
  - Snap recorded MIDI notes to grid
  - Quantize strengths: 1/4, 1/8, 1/16, 1/32, triplets
  - Adjustable quantize amount (0-100%)
  - Apply to selection or entire pattern

- [ ] **Piano Roll View**
  - Graphical note editing as alternative to text cells
  - Visual representation of note lengths and velocities
  - Mouse-based note entry and editing
  - Toggle between tracker grid and piano roll views

- [ ] **Pattern Templates**
  - Save/load pattern templates for quick reuse
  - Template library with categories (drums, bass, chords, etc.)
  - Cross-song template sharing
  - Include FX chains in templates

- [ ] **Sample Trigger**
  - Trigger one-shot audio samples from cells
  - Sample browser with preview
  - Per-cell sample parameters (pitch, volume, start offset)
  - Basic sample slicing

---

## Backlog: Feature Opportunities

### Preset Browser & Layering

- [ ] Add preset browsing UI to editor/REPL
  - TSF already exposes preset metadata via `shared_tsf_get_preset_name()`
  - No UI for browsing, tagging, or layering presets
  - Let musicians audition instruments and build splits/stacks without editing raw program numbers

### Session Capture & Arrangement

- [ ] Elevate shared MIDI event buffer to first-class timeline (`src/shared/midi/events.h`)
  - Currently only feeds export
  - Capture REPL improvisations into clips, arrange them, re-trigger live
  - Similar to Ableton's Session View but text-driven

### Controller & Automation Mapping

- [ ] Map physical MIDI controllers or OSC sources to language variables
  - Tempo, volume, macro parameters
  - Makes Joy/TR7 live-coding sets more expressive
  - Combine with existing Ableton Link transport hooks

### Cross-Language Patch Sharing

- [ ] Create lightweight messaging bus for Alda, Joy, TR7 to exchange motifs
  - Example: Joy macro emits motif that Alda editor picks up and renders with full notation
  - Showcases polyglot nature, keeps multiple buffers in sync

### Real-Time Visualization

- [ ] Expose playback state in loki status bar or via OSC/WebSocket
  - Current measure, active voices, CPU load
  - Visual confirmation when multiple asynchronous schedulers are active
