# Psnd TODO

## Known Bugs

### `:plugin presets` buffer switch not working

**Status:** Unresolved

**Description:** The `:plugin presets` command is intended to open a new scratch buffer displaying all plugin presets, allowing users to scroll through and view the full list. The command executes without errors, but the new buffer does not appear - the editor stays on the original buffer.

**Expected behavior:**
1. User types `:plugin presets` and presses Enter
2. A new buffer opens showing:
   - Plugin name header
   - Total preset count and current preset number
   - Full list of presets with indices (e.g., `*   42: Warm Pad` where `*` marks current)
3. User can scroll through presets, then close buffer with `:q`

**Actual behavior:** Command returns success but display doesn't change. The original file buffer remains visible.

**Root cause analysis:**

The issue appears to be related to how buffer switching interacts with command mode exit:

1. In `command/plugin.c`, the `:plugin presets` handler:
   - Calls `buffer_create(NULL)` to create a new empty buffer
   - Calls `buffer_switch(buf_id)` to switch to the new buffer
   - Gets new context via `preset_ctx = buffer_get_current()`
   - Populates the buffer with preset data via `editor_insert_row()`
   - Sets `preset_ctx->view.mode = MODE_NORMAL`
   - Returns success (1)

2. In `command.c:command_mode_handle_key()`, after the command handler returns:
   - `command_mode_exit(ctx)` is called with the ORIGINAL context
   - This sets `ctx->view.mode = MODE_NORMAL` on the OLD buffer
   - Clears the status message on the OLD buffer

3. In `editor.c` main loop:
   - Each iteration calls `ctx = buffer_get_current()` which SHOULD return the new buffer
   - Calls `editor_refresh_screen(ctx)` which SHOULD render the new buffer

**What was tried:**
1. Verified `buffer_create()` returns valid buffer ID (checked object file symbols)
2. Verified `buffer_switch()` updates `buffer_state.current_buffer_id`
3. Added explicit `preset_ctx->view.mode = MODE_NORMAL` after buffer switch
4. Confirmed all code is compiled in (strings present in binary)
5. Clean rebuild with `BUILD_MINIHOST_BACKEND=ON`
6. Added explicit error checking for `buffer_switch()` return value in plugin.c
7. Created comprehensive unit test `buffer_command_workflow` that simulates the exact workflow - test passes, confirming buffer management works correctly in isolation

**Analysis (2026-01-24):**
The buffer management code is correct. A new unit test in `test_buffers.c` simulates exactly what `:plugin presets` does:
- Create buffer, switch to it, delete initial row, insert content rows
- All operations complete successfully and `buffer_get_current()` returns the correct context

**Systematic investigation of potential causes:**

| Hypothesis | Finding | Evidence |
|------------|---------|----------|
| Minihost plugin | **Not the cause** | Plugin functions are pure read-only queries. Audio runs in separate thread with proper mutex isolation. No editor state modification. |
| Async event queue | **Not the cause** | All handlers call Lua callbacks or evaluate specific buffers by ID. None modify `current_buffer_id`. |
| Terminal rendering | **Not the cause** | Both renderer and VT100 paths work correctly. `command_mode_exit` only modifies mode/status on OLD context. |

**Improvements made:**
- Added explicit error checking for `buffer_switch()` return value
- Added runtime verification that buffer ID matches after switch
- Status messages now include diagnostic info on failure

**Remaining possibilities:**
1. Environment-specific - Bug only manifests with specific plugins or terminal emulators
2. Timing-related - Possible race condition in real-world conditions
3. Already fixed - The bug may have been resolved by other changes

**Ruled out:**
1. ~~`buffer_switch()` may not properly save/restore state between buffers~~ (confirmed working via unit test)
2. ~~The `ctx` passed to command handler may be cached somewhere else~~ (code trace shows ctx is local variable)
3. ~~Screen refresh may be using a stale context pointer~~ (main loop fetches ctx each iteration)
4. ~~Buffer initialization may be incomplete (missing screen dimensions, etc.)~~ (confirmed working via unit test)
5. ~~The command handler's return may trigger additional processing that resets state~~ (no such code exists)
6. ~~Async event queue dispatch has side effects~~ (handlers don't modify buffer state)

**Files involved:**
- `source/core/loki/command/plugin.c` - Command implementation
- `source/core/loki/command.c` - Command execution and mode handling
- `source/core/loki/buffers.c` - Buffer management
- `source/core/loki/editor.c` - Main loop and screen refresh

**Workaround:** Use `:plugin preset <n>` to select presets by number, or `:plugin preset <name>` for partial name matching. The status bar shows current preset info with `:plugin`.

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

- [ ] Extract buffer manager to injectable service
  - Remove global `buffer_state` in `buffers.c`
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

- [ ] Expand editor highlight vocabulary for full tree-sitter support
  - Currently maps ~40 tree-sitter token types to only 9 HL_* constants
  - Missing distinctions:
    - `function` vs `function.builtin` vs `function.call` (all map to HL_KEYWORD1)
    - `variable.builtin` (self/this) vs regular variables (both map to HL_NORMAL)
    - `operator` and `punctuation.*` (map to HL_NORMAL, no color)
    - `constructor`, `namespace`, `label`, `tag` (all map to HL_NORMAL)
    - `keyword.control` vs `keyword.function` vs `keyword.return` (all map to HL_KEYWORD1)
  - Implementation:
    - Expand HL_* constants (add HL_FUNCTION, HL_OPERATOR, HL_PUNCTUATION, HL_VARIABLE_BUILTIN, etc.)
    - Expand `ctx->view.colors[]` array to match
    - Update `capture_to_hl()` in `treesitter.c` for finer mapping
    - Update `syntax_apply_theme_colors()` to map more TOK_* to new HL_* types
  - Files: `internal.h`, `treesitter.c`, `syntax.c`

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
