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

### Architecture

- [x] ~~Extract buffer manager to injectable service~~ **DONE**
  - Added `buffer_manager_t` struct with `*_in()` API variants
  - Global `g_buffer_manager` for backwards compatibility
  - Enables multi-editor and better testability

- [ ] Wrap editor core in standalone service process (optional)
  - Small RPC protocol (stdio JSON or gRPC)
  - Commands: load file, save, apply keystroke, get view state
  - Would enable embedding editor in other applications

## Medium Priority

### Testing

- [ ] Add scanner/lexer unit tests for all languages
  - Alda scanner vulnerable to malformed input
  - Joy/Bog/TR7 lexers untested

- [ ] Add fuzzing infrastructure
  - Parser robustness for malformed input
  - Consider AFL or libFuzzer integration

### Ableton Link Integration

- [ ] **Full Transport Sync** - Start/stop from any Link peer controls all
  - Wire transport callbacks to actually start/stop playback
  - Requires interruptible playback and a "armed for playback" state
  - Most complex - requires rethinking REPL interaction model

### Editor Features

- [ ] Complete command dispatcher for all keybindings
  - Currently `eval_line`, `play_file`, and `quit` are hardcoded in `modal.c`
  - These commands have complex behavior (multi-press confirmation, internal functions)
  - Refactor to allow TOML configuration of these bindings

- [ ] Playback visualization
  - Highlight currently playing region
  - Show playback progress in status bar

- [ ] Tempo tap
  - Tap key to set tempo

- [ ] Metronome toggle

### Test Framework

- [ ] Refactor CLI tests to avoid shell spawning (`tests/cli/test_play_command.c:29-62`)
  - Tests use `system("rm -rf ...")` for cleanup and `system()` to invoke psnd
  - Couples tests to `/bin/sh`, ignores exit codes in some branches
  - Fix: Use `fork`/`execve` directly for binary invocation, `mkdtemp`/`nftw` for temp directory cleanup

- [ ] Add missing test coverage
  - No tests for: editor bridge, Ableton Link callbacks, shared REPL command processor
  - Pointer/string comparisons can silently truncate in test framework

- [ ] Add `ASSERT_GT`, `ASSERT_LT` macros

- [ ] Add test fixture support (setup/teardown)

- [ ] Add memory leak detection hooks

---

## Low Priority

### Code Consolidation

- [ ] Extract shared REPL loop skeleton
  - ~150 lines of help functions still duplicated per language
  - Interactive loop structure still duplicated (could use callback pattern)

- [ ] Centralize platform CMake logic
  - Platform detection repeated in 6+ CMakeLists.txt files
  - Create `psnd_platform.cmake` module

### Platform Support

- [ ] Windows support
  - Editor uses POSIX headers: `termios.h`, `unistd.h`, `pthread.h`
  - Options: Native Windows console API, or web editor using CodeMirror/WebSockets

### Editor Features

- [ ] Split windows
  - Already designed for in `editor_ctx_t`
  - Requires screen rendering changes

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

### Documentation

- [ ] API reference generation
  - Consider Doxygen for generated docs

- [ ] Contributing guide

- [ ] Build troubleshooting
  - Platform-specific guidance

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

---

## Add Languages

- [ ] bytebeat (see: <https://dollchan.net/bytebeat>)
- [ ] funcbeat
- [ ] drumbeat (see: <https://wavepot.com>)

## Tracker Enhancements

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

## Feature Opportunities

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
