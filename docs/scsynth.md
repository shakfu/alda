# scsynth Integration Options for psnd

This document evaluates the approaches for integrating SuperCollider's synthesis server (`scsynth`) as a psnd audio backend. It covers architecture, trade-offs, and implementation details for each option.

## Background

### What is scsynth?

scsynth is the real-time audio synthesis server from SuperCollider. It is a headless process controlled entirely via OSC (Open Sound Control) messages over UDP or TCP. It loads UGen plugins (`.scx` shared libraries) that provide DSP building blocks, and executes SynthDefs (pre-compiled synthesis graphs) to produce audio.

- Default OSC port: 57110

- Protocol: OSC 1.0 over UDP or TCP

- Audio: CoreAudio (macOS), JACK/ALSA (Linux), PortAudio (Windows)

### How psnd backends work today

psnd routes note events through a priority chain in `context.c`:

```
Minihost (VST3/AU) > Csound > Built-in Synth (TSF/FluidSynth) > MIDI
```

Each backend is a global singleton with ref-counted enable/disable. The routing uses early-return: only the highest-priority enabled backend receives each event. Panic broadcasts to all backends.

Key dispatch surface (from `context.h`):

```c
void shared_send_note_on(SharedContext* ctx, int channel, int pitch, int velocity);
void shared_send_note_on_freq(SharedContext* ctx, int channel, double freq,
                              int velocity, int midi_pitch);
void shared_send_note_off(SharedContext* ctx, int channel, int pitch);
void shared_send_program(SharedContext* ctx, int channel, int program);
void shared_send_cc(SharedContext* ctx, int channel, int cc, int value);
void shared_send_panic(SharedContext* ctx);
```

Backends implement a standard interface: `init`, `cleanup`, `enable`, `disable`, `is_enabled`, `send_note_on`, `send_note_off`, `send_program`, `send_cc`, `all_notes_off`. Some also support `send_pitch_bend`, `send_note_on_freq`, and audio rendering callbacks.

---

## Option A: OSC Client (Out-of-Process)

Connect to an externally-running scsynth instance via OSC over UDP. psnd acts as a pure client, similar to how `sclang`, `Overtone`, `TidalCycles`, and `FoxDot` interact with scsynth.

### Architecture

```
  psnd (C process)              scsynth (separate process)
  +-----------------+           +---------------------+
  | SharedContext   |   UDP     | OSC command handler  |
  | scsynth_backend |---------->| SynthDef engine      |
  | (liblo client)  |   :57110  | UGen plugin host     |
  +-----------------+           | Audio driver (CoreAudio/JACK) |
                                +---------------------+
```

psnd sends OSC messages. scsynth owns audio output.

### Event Translation

```
psnd event                     OSC message
-----------                    -----------
note_on(ch, pitch, vel)   -->  /s_new "default" <node_id> 0 1
                                 "freq" <hz> "amp" <0..1> "gate" 1

note_on_freq(ch, freq,    -->  /s_new "default" <node_id> 0 1
  vel, midi_pitch)               "freq" <freq> "amp" <0..1> "gate" 1

note_off(ch, pitch)       -->  /n_set <node_id> "gate" 0

program(ch, program)      -->  Update SynthDef name lookup table
                                (next /s_new uses different SynthDef)

cc(ch, cc, value)         -->  /n_set <node_id> "cc<N>" <0..1>
                                (or map to named SynthDef params)

panic()                   -->  /g_freeAll 1
                                (free all nodes in default group)
```

Frequency conversion (for standard note_on):

```c
double freq = 440.0 * pow(2.0, (pitch - 69) / 12.0);
double amp  = velocity / 127.0;
```

### Active Note Tracking

Maps `(channel, pitch)` pairs to scsynth node IDs:

```c
#define SC_MAX_NOTES 256

typedef struct {
    int channel;
    int pitch;
    int node_id;
} ScActiveNote;

typedef struct {
    lo_address server;          /* liblo address for scsynth */
    int connected;
    int enabled;
    int ref_count;
    int next_node_id;           /* Monotonically increasing node allocator */
    char synthdef_name[64];     /* Current SynthDef (default: "default") */
    char synthdef_map[128][64]; /* Per-program SynthDef name mapping */
    ScActiveNote notes[SC_MAX_NOTES];
    int note_count;
    sc_mutex_t mutex;
} ScSynthBackend;
```

Node ID allocation: start at 1000, increment per note. Wraps at INT_MAX. scsynth handles ID collisions gracefully (old nodes are freed).

### Connection Lifecycle

```
:sc                        Connect to 127.0.0.1:57110
:sc host:port              Connect to specific address
  |
  +--> Send /status         Check server is alive
  +--> Send /notify 1       Subscribe to node notifications
  +--> Send /g_new 1 0 0    Create default group (if not exists)
  +--> Send /dumpOSC 0      Disable OSC dump (quiet mode)
  |
  Ready for note events.
  |
:sc-disconnect
  +--> Send /g_freeAll 1    Kill all active synths
  +--> Send /notify 0       Unsubscribe
```

### REPL Commands

```
:sc                         Connect to scsynth (default 127.0.0.1:57110)
:sc host:port               Connect to scsynth at address
:sc-synth <name>            Set SynthDef for note dispatch
:sc-synth <program> <name>  Map program number to SynthDef name
:sc-status                  Query server status (/status -> /status.reply)
:sc-disconnect              Disconnect from scsynth
:sc-free                    Free all synths (/g_freeAll)
:sc-send <path> [args...]   Send raw OSC message (advanced)
```

### Implementation Estimate

- New file: `scsynth_backend.c` (~400-600 lines)

- Header additions to `audio.h` (~40 lines)

- Routing additions to `context.c` (~30 lines)

- REPL command additions to `repl_commands.c` (~80 lines)

- SharedContext: add `int scsynth_enabled` flag

### Dependencies

- `liblo` (already in psnd for OSC support, guarded by `PSND_OSC`)

- No new compile-time or runtime dependencies

### Build Integration

Conditional on existing `PSND_OSC` flag, or a new `BUILD_SCSYNTH_BACKEND` flag. Since it only requires `liblo` (already present), it could be always-on when OSC is enabled.

### Advantages

- Minimal implementation (~500 lines of C)

- Zero new dependencies (uses existing `liblo`)

- User gets full SuperCollider ecosystem (sclang, SC IDE, `sc3-plugins`)

- scsynth manages its own audio driver (no `miniaudio` integration needed)

- Any existing SynthDef works (vast community library)

- Multiple psnd instances can share one scsynth

- Clean process separation (scsynth crash does not crash psnd)

- Microtuning support via direct frequency parameter

### Disadvantages

- User must start scsynth separately (or have SC IDE running)

- Network latency (UDP on localhost: ~0.1ms, negligible in practice)

- No audio routing back through psnd (cannot mix with other backends)

- Cannot apply psnd-side post-processing

- Depends on scsynth having suitable SynthDefs loaded

- No offline/NRT rendering through this path

### SynthDef Strategy

scsynth always has a built-in `default` SynthDef (VarSaw + Linen envelope, params: `out`, `freq`, `amp`, `pan`, `gate`). Sound comes out with zero configuration.

For richer sounds, psnd can optionally ship a `.scd` file containing recommended SynthDef definitions. Users evaluate it in sclang:

```
// In sclang:
"/path/to/psnd/extras/psnd_synthdefs.scd".load;
```

This is documentation/convenience, not a runtime dependency.

Alternatively, load pre-compiled `.scsyndef` files into scsynth on connect:

```c
// Send /d_load for each .scsyndef in ~/.psnd/synthdefs/
shared_osc_send_to(host, port, "/d_loadDir", "s", synthdef_dir);
```

---

## Option B: Embedded via libscsynth (In-Process)

Link against `libscsynth.a` and run the synthesis engine inside the psnd process, similar to how the Csound backend embeds libcsound.

### Architecture

```
  psnd (single process)
  +--------------------------------------------------+
  |  SharedContext                                    |
  |  +---------------------------------------------+  |
  |  | ScSynth Embedded Backend                    |  |
  |  |                                             |  |
  |  |  World* world = World_New(&options);        |  |
  |  |  World_OpenUDP(world, "127.0.0.1", 0);      |  |
  |  |  World_SendPacket(world, size, osc_data, fn);| |
  |  |                                             |  |
  |  |  UGen plugins loaded from plugin_path       |  |
  |  |  SynthDefs loaded from synthdef_dir         |  |
  |  +---------------------------------------------+  |
  |                                                   |
  |  scsynth manages its own audio via CoreAudio/JACK |
  +--------------------------------------------------+
```

### C API (from SC_WorldOptions.h)

```c
// Lifecycle
World* World_New(WorldOptions* options);
void   World_Cleanup(World* world, bool unload_plugins);
void   World_WaitForQuit(World* world, bool unload_plugins);

// Network (for external sclang connections)
int  World_OpenUDP(World* world, const char* bindTo, int port);
int  World_OpenTCP(World* world, const char* bindTo, int port,
                   int maxConnections, int backlog);

// Direct packet injection (bypass network)
bool World_SendPacket(World* world, int size, char* data, ReplyFunc func);
bool World_SendPacketWithContext(World* world, int size, char* data,
                                ReplyFunc func, void* context);

// Utility
void SetPrintFunc(PrintFunc func);
int  World_CopySndBuf(World* world, uint32 index, SndBuf* outBuf,
                      bool onlyIfChanged, bool* didChange);
```

The `World_SendPacket` function accepts raw OSC byte buffers. This means the event translation layer is identical to Option A -- construct OSC messages with liblo (or oscpack, or raw byte packing), then inject them via `World_SendPacket` instead of sending over UDP.

### WorldOptions Configuration

```c
WorldOptions options;
// Zero-init with defaults (C++ struct with member initializers)

options.mPreferredSampleRate = 44100;
options.mNumOutputBusChannels = 2;
options.mNumInputBusChannels = 0;      // No audio input needed
options.mBufLength = 64;               // Block size
options.mRealTimeMemorySize = 8192;    // 8MB RT memory
options.mMaxNodes = 1024;
options.mMaxGraphDefs = 1024;
options.mVerbosity = -2;               // Very quiet
options.mRendezvous = false;           // No Bonjour
options.mUGensPluginPath = plugin_path; // Path to .scx files
options.mLoadGraphDefs = 1;            // Auto-load SynthDefs on boot
```

### Initialization Flow

```c
int shared_scsynth_init(void) {
    WorldOptions opts;
    // configure opts...

    g_sc.world = World_New(&opts);
    if (!g_sc.world) return -1;

    // Optionally open UDP so sclang can also connect
    World_OpenUDP(g_sc.world, "127.0.0.1", 57110);

    g_sc.initialized = 1;
    return 0;
}
```

### Event Dispatch

Same OSC message construction as Option A, but delivered via:

```c
void shared_scsynth_send_note_on(int channel, int pitch, int velocity) {
    char buf[256];
    // Build OSC /s_new message into buf...
    int size = build_osc_s_new(buf, sizeof(buf), synthdef, node_id,
                                freq, amp);
    World_SendPacket(g_sc.world, size, buf, reply_func);
}
```

This bypasses the network stack entirely. Events go directly into the server's command FIFO.

### Audio Output

scsynth manages its own audio device internally (CoreAudio on macOS, JACK on Linux). When embedded, it still opens its own audio device -- it does **not** route audio through psnd's miniaudio pipeline. This is different from the Csound backend, which uses host-implemented audio I/O.

It is theoretically possible to use scsynth's NRT (non-real-time) mode to render to buffers and pull samples into psnd's audio callback, but this is not how libscsynth is designed to be used and would require significant custom work.

### Plugin and SynthDef Paths

The embedded World needs to know where to find:

1. **UGen plugins** (`.scx` files): Set via `options.mUGensPluginPath`

2. **SynthDefs** (`.scsyndef` files): Loaded from the SC synthdef directory or via `/d_load` / `/d_recv` packets

These paths must be configured at init time or resolved from the user's SuperCollider installation.

### Implementation Estimate

- New file: `scsynth_backend.c` (~800-1200 lines)

- Header additions to `audio.h` (~50 lines)

- Routing additions to `context.c` (~30 lines)

- REPL command additions (~100 lines)

- CMake: find libscsynth, SC headers, link C++ runtime

- OSC message builder (shared with Option A, ~200 lines)

### Dependencies

**Compile-time:**
- `libscsynth.a` (1.1 MB) -- static library from SC build

- SC headers: `SC_WorldOptions.h`, `SC_Reply.h`, `SC_Types.h`, `SC_Export.h`

- C++ standard library (libscsynth is C++ internally)

**Runtime:**
- UGen plugin directory (26 core `.scx` files, ~1.8 MB)

- libsndfile (dynamic dependency of scsynth)

- CoreAudio/CoreServices/Foundation frameworks (macOS)

- Optionally: sc3-plugins (~100+ additional `.scx` files)

**Build system impact:**
- Must link C++ runtime (`-lc++` on macOS, `-lstdc++` on Linux)

- Must find or bundle SC headers

- psnd binary grows by ~1.1 MB (libscsynth) + plugin directory on disk

- New CMake flag: `-DBUILD_SCSYNTH_BACKEND=ON`

- Requires SC source tree or installed headers at build time

### Advantages

- No external process to manage (user types `:sc` and it works)

- `World_SendPacket` bypasses network stack (lower latency than UDP)

- Can open UDP port so sclang can also connect to the same server

- Self-contained distribution possible (bundle scsynth + plugins + defs)

- Process lifecycle is fully controlled by psnd

### Disadvantages

- Significant new dependencies (libscsynth, C++ runtime, libsndfile, platform audio frameworks)

- Binary size increase (~1.1 MB static lib + ~1.8 MB core plugins minimum)

- scsynth owns its audio device (cannot route through psnd's miniaudio)

- WorldOptions is a C++ struct -- requires C++ compilation unit or wrapper

- libscsynth API stability caveat: "API might change across minor versions" (noted in SC_WorldOptions.h)

- scsynth crash takes down entire psnd process

- Complex build system integration (find SC headers, plugins, link flags)

- Plugin path resolution varies by platform and installation method

---

## Option C: Managed Process (Spawn + OSC)

psnd spawns scsynth as a child process and communicates via OSC. A middle ground between A (pure client) and B (full embedding).

### Architecture

```
  psnd (parent process)
  +-----------------+          scsynth (child process)
  | SharedContext    |  fork/   +---------------------+
  | scsynth_backend |  exec    | Audio synthesis      |
  | (liblo client)  |--------->| UGen plugins         |
  |                 |   UDP    | Own audio device      |
  | Process mgmt:   |<-------->| Responds to OSC      |
  |  spawn/kill     |  :57110  +---------------------+
  +-----------------+
```

### Process Management

```c
int shared_scsynth_boot(const char* scsynth_path, int port) {
    // Resolve scsynth binary
    // Build argv: scsynth -u <port> -V -2 -R 0 -U <plugin_path>
    // fork + exec (posix) or CreateProcess (Windows)
    // Wait for /status.reply to confirm server is ready
    // Connect OSC client
}

void shared_scsynth_quit(void) {
    // Send /quit via OSC
    // waitpid with timeout
    // SIGTERM/SIGKILL if unresponsive
}
```

### scsynth Discovery

Resolve the scsynth binary path using a search order:

```
1. User-specified path:   :sc-boot /path/to/scsynth

2. Environment variable:  $SCSYNTH_PATH

3. Config file:           ~/.psnd/config  ->  scsynth_path = ...

4. PATH lookup:           which scsynth

5. Known locations:
   macOS:  /Applications/SuperCollider.app/Contents/Resources/scsynth
           ~/src/supercollider/build/server/scsynth/scsynth
   Linux:  /usr/bin/scsynth
           /usr/local/bin/scsynth
```

### REPL Commands

```
:sc-boot                    Spawn scsynth, connect
:sc-boot /path/to/scsynth   Spawn specific binary
:sc-boot port=57200         Spawn on non-default port
:sc-quit                    Send /quit, wait for exit
:sc                         Connect to already-running scsynth (same as A)
```

All other commands (`:sc-synth`, `:sc-status`, etc.) are identical to Option A since the communication layer is the same.

### Implementation Estimate

- Everything from Option A (~500 lines)

- Process management: ~200-300 additional lines

- scsynth path discovery: ~100 lines

- Total: ~800-900 lines

### Dependencies

- liblo (existing, for OSC)

- `posix_spawn` or `fork`/`exec` (POSIX), `CreateProcess` (Windows)

- scsynth binary must be installed somewhere on the system

### Advantages

- Better UX than pure client: `:sc-boot` is one command

- All advantages of Option A (full SC ecosystem, process isolation)

- scsynth crash does not crash psnd

- No compile-time SC dependency

- Can detect and reuse already-running scsynth instances

### Disadvantages

- Process management complexity (spawn, health check, cleanup, orphan prevention)

- Must handle scsynth not being installed (graceful error)

- Platform-specific process spawning code

- Port conflict handling (what if 57110 is taken?)

- Startup latency (scsynth takes ~1-2 seconds to boot and load plugins)

- Still requires SynthDefs to be available

---

## Option D: Hybrid (Embed + External Client)

Combine Options B and A: attempt to embed libscsynth if available at compile time; fall back to OSC client for external scsynth otherwise. This is how the Csound backend works (embedded if compiled in, otherwise stubs).

### Architecture

```c
#ifdef BUILD_SCSYNTH_EMBEDDED
  // Use World_New / World_SendPacket
  // Bundle plugins and SynthDefs
#else
  // Use liblo OSC client to external scsynth
  // No SC build dependency
#endif
```

### Implementation

The OSC message construction is identical in both modes. The only difference is the transport:

```c
static void sc_send_osc(const char* buf, int size) {
#ifdef BUILD_SCSYNTH_EMBEDDED
    if (g_sc.world) {
        World_SendPacket(g_sc.world, size, (char*)buf, reply_func);
        return;
    }
#endif
    if (g_sc.server) {
        lo_send_message(g_sc.server, buf, size);
    }
}
```

### Advantages

- Maximum flexibility: works with or without SC build dependency

- Embedded mode for distribution; client mode for development

- Shared code between modes

### Disadvantages

- Two code paths to test and maintain

- Embedded mode inherits all disadvantages of Option B

- Build matrix complexity

---

## Comparison Matrix

| Factor                    | A: OSC Client | B: Embedded  | C: Managed     | D: Hybrid      |
|---------------------------|---------------|--------------|----------------|----------------|
| Implementation effort     | Low (~500 LOC)| High (~1K LOC)| Medium (~900)  | High (~1.2K)   |
| New dependencies          | None          | libscsynth, C++, libsndfile | None | Conditional    |
| Binary size impact        | 0             | +3 MB min    | 0              | 0 or +3 MB     |
| Build complexity          | Trivial       | Significant  | Trivial        | Moderate        |
| User setup required       | Start scsynth | None (or find plugins) | Install SC  | Varies         |
| Startup UX                | Manual        | Automatic    | One command     | Automatic or manual |
| Latency                   | ~0.1ms (UDP)  | ~0 (direct)  | ~0.1ms (UDP)   | Varies         |
| Process isolation         | Yes           | No           | Yes            | Varies         |
| sclang co-use             | Natural       | Possible (open UDP) | Natural  | Possible       |
| Crash isolation           | Yes           | No           | Yes            | Varies         |
| Microtuning (freq)        | Yes           | Yes          | Yes            | Yes            |
| Audio back-routing        | No            | No*          | No             | No             |
| Offline/NRT rendering     | No            | Possible     | Possible       | Possible       |
| Platform portability      | Good          | Complex      | Good           | Complex        |

*scsynth always manages its own audio device in all options.

---

## SynthDef Considerations

All options share the same SynthDef requirements. scsynth needs SynthDef programs to produce sound. Strategies for providing them:

### 1. Rely on scsynth's built-in `default` SynthDef

Always available. Params: `out`, `freq`, `amp`, `pan`, `gate`. Uses a VarSaw oscillator with a Linen envelope. Thin but functional.

### 2. Ship pre-compiled `.scsyndef` files

Place in `.psnd/synthdefs/` and load on connect via `/d_loadDir`. Users can add their own files to the same directory.

Candidate SynthDefs for a shipped set:
- `psnd_sine` -- clean sine, good for testing

- `psnd_saw` -- filtered sawtooth, general purpose

- `psnd_pad` -- detuned oscillators with slow attack

- `psnd_pluck` -- Karplus-Strong physical model

- `psnd_fm` -- FM synthesis bell/keys

- `psnd_bass` -- low-passed square wave

- `psnd_perc` -- percussive envelope, noise + tone

- `psnd_string` -- DWGPlucked (requires sc3-plugins)

### 3. Ship sclang source alongside compiled defs

Include `.scd` source in `extras/synthdefs/` for users to modify and recompile. This is documentation, not a runtime dependency.

### 4. Program change mapping

Map MIDI program numbers to SynthDef names:

```
program 0  -->  "psnd_saw"     (default instrument)
program 1  -->  "psnd_sine"
program 2  -->  "psnd_pad"
...
```

Configurable via `:sc-synth <program> <name>` or a config file.

### 5. Rely on user's SC environment

If the user has sclang running, their SynthDefs are already loaded in scsynth. psnd just needs to know the SynthDef name to target. This is the zero-effort path for SC-experienced users.

---

## Recommended Approach

**Start with Option A (OSC Client), design for Option C (Managed Process) as a future enhancement.**

Rationale:

1. **Option A is the minimum viable integration.** ~500 lines, zero new dependencies, uses existing liblo. It validates the entire concept before investing in process management or embedding.

2. **The OSC message layer is shared across all options.** Building Option A first creates the translation layer (MIDI events to scsynth OSC commands, active note tracking, SynthDef mapping) that every other option reuses.

3. **Option C is a natural extension of A.** Adding `:sc-boot` to spawn scsynth is additive -- the connection and event code is identical.

4. **Option B (embedding) fights scsynth's architecture.** scsynth is designed as a server. It manages its own audio device, its own thread pool, its own memory allocator. Embedding it gains little over connecting via localhost UDP, while adding substantial build complexity and a C++ dependency. The Csound embedding model works because Csound was designed for host-implemented audio I/O; scsynth was not.

5. **Target audience matters.** Users who want scsynth likely already have SuperCollider installed. Requiring them to start the server is a familiar workflow (identical to Overtone, TidalCycles, FoxDot). The friction is acceptable and expected.

### Suggested priority chain position

```
Minihost > Csound > Built-in Synth (TSF/Fluid) > scsynth > MIDI
```

scsynth sits below the in-process synthesis engines (which have tighter audio integration) and above raw MIDI (which requires external hardware or software).

### Implementation phases

**Phase 1: OSC Client (Option A)**
- `scsynth_backend.c` with connect/disconnect/send

- Active note tracking with node ID allocation

- Basic SynthDef name configuration

- REPL commands: `:sc`, `:sc-synth`, `:sc-status`, `:sc-disconnect`

- SharedContext flag: `scsynth_enabled`

- Priority chain integration in `context.c`

**Phase 2: SynthDef Management**
- Ship 5-8 pre-compiled `.scsyndef` files

- `/d_loadDir` on connect

- Program-to-SynthDef mapping

- `:sc-synth <program> <name>` command

**Phase 3: Managed Process (Option C)**
- `:sc-boot` command with scsynth discovery

- Process spawn/kill lifecycle

- Health monitoring via `/status` polling

- Graceful degradation if scsynth is not installed

---

## References

- SuperCollider Server Command Reference: https://doc.sccode.org/Reference/Server-Command-Reference.html

- SuperCollider Server Architecture: https://doc.sccode.org/Reference/Server-Architecture.html

- liblo (OSC library): https://liblo.sourceforge.net/

- sc3-plugins: https://github.com/supercollider/sc3-plugins

- SynthDef file format: https://doc.sccode.org/Reference/Synth-Definition-File-Format.html

- Sonic Pi's scsynth integration (prior art): https://github.com/sonic-pi-net/sonic-pi

- Overtone (Clojure SC client, prior art): https://github.com/overtone/overtone
