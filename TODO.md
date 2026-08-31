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

### Phrase arrays leaked on tracker engine teardown

**Status:** Open

`tracker_engine.c:659` frees `engine->recent_by_track` but not the phrase arrays
each entry owns:

```c
if (engine->recent_by_track) {
    /* TODO: free phrase arrays */
    free(engine->recent_by_track);
```

Bounded per engine instance, so it does not grow during a session, but it does
leak on every engine create/destroy cycle and shows up under ASan.

---

## High Priority

### Licensing & Third-Party Attribution

- [ ] **Resolve the mongoose license conflict before distributing web builds**
  - psnd is GPL-3.0. Mongoose (`source/thirdparty/mongoose-7.20/`) is
    **GPL-2.0-only or commercial** — GPL-2.0-only is incompatible with
    distributing a combined GPL-3.0 work.
  - Affects the `web`, `tsf-web`, `fluid-web` and `fluid-csound-web` variants,
    which CI now builds and uploads as artifacts.
  - Options: obtain a commercial mongoose license, relicense psnd to
    GPL-2.0-or-later, or replace mongoose with a permissively licensed HTTP/WS
    server for the web host.
  - This is a legal question, not a technical one — get a definitive answer
    before publishing binaries of those variants.

- [ ] Add license texts for all vendored dependencies
  - `docs/licenses/` currently contains only `KILO-LICENSE`; there are 23
    dependencies under `source/thirdparty/`.
  - Several are LGPL (Csound, FluidSynth, libsndfile, liblo) or GPL-2.0+
    (Ableton Link), so attribution is an obligation, not a courtesy.

- [ ] Add `THIRD-PARTY.md` recording each dependency's upstream URL and pinned version
  - 67 MB is vendored with no submodules, so there is currently no way to tell
    what version anything is or to audit for upstream security fixes.

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
  - [x] ~~Authentication (required before exposing to network)~~ **DONE**
    - Binds `127.0.0.1` by default; `--web-host ADDR` is an explicit opt-in
    - Random per-session token required on `/ws` and the `/api` endpoints
    - Cross-origin WebSocket handshakes rejected (`Origin` check)
    - `LUA_SANDBOX` defaults ON when `BUILD_WEB_HOST=ON`
  - [ ] Multiple client support (currently single WebSocket connection)
    - `WebHostData.ws_conn` is one pointer; a second client overwrites it
    - See "Support multiple editor sessions in one process" under Low Priority
  - [ ] Session persistence (save/restore editor state across restarts)
  - [ ] Test coverage for the access-control logic (see Code Coverage)

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

- [ ] **Merge the duplicated REPL line editors**
  - `source/core/repl.c` (648 lines) and `source/core/loki/repl_line_editor.c`
    (823 lines) are near-identical implementations of the same line editor;
    `source/core/CMakeLists.txt:334-337` picks `repl.c` as the fallback when
    loki is not built.
  - The duplication has already caused a divergent fix: the `strcpy` hardening
    was applied to one copy's `ARROW_UP` path only, and the other copy had
    neither branch fixed (both are now bounded, but the asymmetry will recur).
  - Extract the shared core into one file with the linenoise-backed path behind
    `#ifdef LOKI_USE_LINENOISE` — the mechanism `repl_line_editor.c` already
    uses internally — and delete the duplicate.
  - Also collapses the duplicate headers `source/core/repl.h` (126 lines) and
    `source/core/loki/repl.h` (125 lines).

- [ ] Define `MAX_INPUT_LENGTH` exactly once
  - `source/core/repl.h:20` defines it as **1024**;
    `source/core/loki/repl_helpers.c:31` defines it as **4096** behind an
    `#ifndef`, so the effective value depends on include order per translation
    unit. Folds into the REPL merge above.

- [ ] Add checked-allocation wrappers and migrate incrementally
  - A heuristic scan (allocation whose next line is not a NULL check) flags
    ~255 sites across `source/core` and `source/langs`. Many are false
    positives, but the discipline is not uniform outside `loki/core.c`.
  - Add `psnd_xmalloc`/`psnd_xcalloc`/`psnd_xstrdup` to `shared/` rather than
    auditing every site by hand.

- [ ] Reconsider `exit(1)` on allocation failure in `loki/core.c`
  - Sites at `core.c:252, 282, 293, 368, 380, 396` and elsewhere call
    `perror("Out of memory"); exit(1);` (inherited from kilo).
  - Two problems: unsaved buffer contents are lost with no recovery attempt,
    and `libloki` is buildable as a shared library (`LOKI_BUILD_SHARED`), where
    calling `exit()` from library code is not acceptable.
  - The overflow guards preceding these allocations are correct — only the
    failure response needs changing.

- [ ] Give `tracker_notes_to_string()` a buffer-size parameter
  - `tracker_plugin_notes.h:86` takes a bare `char* buffer` and
    `tracker_plugin_notes.c:186` writes into it with `sprintf`. Bounded in
    practice (max output `"A#-1"`), but the API invites a future overflow.

- [ ] Replace `atoi` with `strtol` in config and command parsing
  - 29 call sites. `atoi` silently returns 0 on garbage and is undefined on
    overflow, so malformed user input becomes a valid-looking 0 instead of an
    error.

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

- [ ] `host_web.c` (888 lines) - **no tests at all**
  - The only network-facing component in the project, and the least tested.
  - Now carries access-control logic (session token, `Origin` check) that
    should not be allowed to regress silently: cover token accept/reject,
    cross-origin rejection, and loopback-by-default binding.

- [ ] `host_webview.cpp` - no tests

- [ ] `tr7/impl/repl.c` (975 lines) - no tests
  - TR7 coverage is `test_reader.c` and `test_music.c` only.

- [ ] `mhs/vfs.c` (1107 lines) - no tests

- [ ] Make disabled-backend tests visibly skip rather than silently pass
  - `shared_csound_backend_tests`, `shared_fluid_backend_tests` and
    `shared_minihost_backend_tests` compile to stubs and pass in ~0.02s when
    their backend is off, so a green local run says nothing about them.
  - Report them as skipped (`ctest` `SKIP_RETURN_CODE`) so the gap is legible.

**CI hardening:**

- [ ] Add an AddressSanitizer job
  - `-DPSND_ENABLE_ASAN=ON` is already wired and the suite runs in ~2.5s, so
    this is nearly free. Would likely surface more of the allocation issues
    listed under Stability & Robustness.

- [ ] Add a `-Werror` job over first-party targets only
  - `-Wall -Wextra -Wpedantic` is on for every first-party target but nothing
    is `-Werror`, so warnings can accumulate unnoticed. Keep it to one
    dedicated job so local builds do not break on new compiler versions, and
    so the 23 vendored dependencies stay exempt.

- [ ] Add a coverage job
  - `-DPSND_ENABLE_COVERAGE=ON` is wired but never exercised; would replace the
    hand-maintained coverage estimates in this section with real numbers.

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

- [ ] Fix remaining stale path references in docs
  - The source tree moved from `src/` to `source/core/`; `new_lang.md` and
    `docs/README.md` have been corrected, these have not:
  - `docs/refactor.md` - ~30 dead `src/...` paths (whole document predates the
    reorganization; consider marking historical like `design_review.md`)
  - `docs/scsynth.md` - `src/supercollider/...`, `.psnd/synthdefs/`, `.psnd/config`
  - `docs/LANG_IMPL_COMPARISON.md` - `source/langs/tr7/dispatch.c`,
    `.psnd/mhs_history`, `source/core/loki/syntax/lang_*`
  - `docs/language-extension-api.md` - refers to `.psnd/languages/alda.lua`;
    the real file is `.psnd/languages/alda.toml`

- [ ] Document the threading contract for the audio backend globals
  - `g_tsf`, `g_fluid`, `g_cs`, `g_link`, `g_async`, `g_queue` are reachable
    from both the async playback thread and the UI thread. The locking
    discipline works but is undocumented, which makes it easy to break.

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

- [ ] Split the largest first-party files
  - `loki/lua.c` (3644 lines) mixes binding registration, sandbox policy
    (`:2923`) and UI output — separate registration from implementation.
  - `tracker/tracker_view.c` (3059 lines) already has `_terminal`, `_json`,
    `_undo`, `_clipboard` and `_theme` siblings; the remaining candidate is the
    `strcmp` command-dispatch chain around `:2340`, which wants a table.
  - `joy/impl/joy_primitives.c` (5173 lines) is a primitive table and is
    arguably fine as-is.

- [ ] Reduce the vendored dependency footprint
  - 67 MB across 23 dependencies with no submodules; `.git` is 16 MB.
  - `tree-sitter-grammars` alone is 23 MB and is the obvious candidate for
    `FetchContent` instead of vendoring.
  - Depends on `THIRD-PARTY.md` (High Priority) to know what versions are pinned.

- [ ] Support multiple editor sessions in one process
  - Blocked by ~25 file-scope globals in `loki/` and `shared/` (`g_config`,
    `g_commands`, `g_current_lang`, the audio singletons, ...).
  - Prerequisite for the web host's "multiple client support" item, since
    `WebHostData.ws_conn` is a single connection pointer today.

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
