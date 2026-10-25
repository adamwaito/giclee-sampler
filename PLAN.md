# Giclee Sampler — Plan

Status: **Phase 1 complete.** Phase 2 is next. Repo: https://github.com/adamwaito/giclee-sampler

Giclee is an open-source automatic multisampler (hardware over MIDI, hosted VST3/AU, and non-MIDI manual capture) with a companion sample-player plugin. It is built on JUCE 9 with a WebView UI, CMake, and targets macOS and Windows.

How to use this file: every phase has checkboxes. Tick them as work lands, and keep **Current state** up to date so a new session can carry on from it.

## Current state

- Phase in progress: none. **Phase 1 is complete**; Phase 2 hasn't started.
- Last session: 2026-10-25. Phase 1 built, pushed to GitHub (public), and CI is green on macOS (universal), Windows (x64) and the frontend check.
- Next step: start Phase 2 (data model, file IO, synthetic signals).
- Not yet verified by eye:
  - The Sampler window's rendering. The app loads the embedded UI and RPC calls arrive (confirmed from the debug log), but the screenshot needs screen-recording permission.
  - The Player editor in a DAW.
  - The Windows build in general.
- Notes for future sessions:
  - `giclee_engine`, `giclee_webui_core` and `giclee_webui` are CMake INTERFACE (source) libraries. Their sources compile into each final target together with the JUCE modules. A shared static library would compile the JUCE modules twice and duplicate their symbols. `giclee_dsp` is a normal static library with no JUCE.
  - Frontend: one Vite project (`ui/`); `--mode sampler|player` picks the app. CMake runs `npm ci` and `npm run build:<app>`, zips the output and embeds it as `GicleeUI::ui_zip`.
  - Dev loop: in Debug builds, setting `GICLEE_UI_DEV_URL=http://localhost:5173/` loads the UI from the Vite dev server.
  - JUCE 9 has no native CLAP support (checked 9.0.4), so CLAP stays in Phase 12 via clap-juce-extensions.
  - Windows: the WebView2 NuGet package is downloaded at configure time (`cmake/GicleeWebView2.cmake`) and linked statically.

---

## 1. Architecture

### 1.1 Layers

```
┌───────────────────────────────┐   ┌───────────────────────────────┐
│ Giclee Sampler (standalone)   │   │ Giclee Player (VST3/AU/SA)    │
│  WebView UI  ⇄  AppController │   │  WebView UI ⇄ PluginProcessor │
└──────────────┬────────────────┘   └──────────────┬────────────────┘
               │                                   │
┌──────────────▼───────────────────────────────────▼────────────────┐
│ giclee_engine   (C++, depends on JUCE audio/MIDI modules, no GUI) │
│  capture sources · sampling scheduler · disk writer · plugin host │
│  pipeline runner · SFZ/WAV/JSON IO · playback engine · profiles   │
└──────────────────────────────┬────────────────────────────────────┘
                               │
┌──────────────────────────────▼────────────────────────────────────┐
│ giclee_dsp      (pure C++20, NO JUCE, NO UI)                      │
│  onset · envelope · pitch (MPM/YIN) · loudness · slicing ·        │
│  loop finder · crossfade · quality score · classifier · resample  │
└───────────────────────────────────────────────────────────────────┘
```

**Change from the brief:** I'm splitting the "shared core" into two libraries.
- `giclee_dsp` has no JUCE dependency at all. It works on plain `float*` / `std::span<const float>` buffers. It's fast to compile, simple to unit test, and could be reused outside JUCE (in a CLI, or bound to Python for tuning loop parameters).
- `giclee_engine` holds everything that needs JUCE: audio and MIDI devices, plugin hosting, file IO, threads. It still has no GUI code.

The playback engine lives in `giclee_engine` because both targets use it. The Sampler needs it to audition loops while they're being edited, and the Player is built on it. That means the loop you hear in the review UI is exactly the loop the Player will play.

### 1.2 Targets

| Target | Kind | Notes |
|---|---|---|
| `GicleeSampler` | JUCE GUI app | MIDI out, audio in, hosts VST3/AU, manual mode, review UI |
| `GicleePlayer` | Plugin: VST3, AU, Standalone | Loads Giclee SFZ sets and basic third-party SFZ |
| `giclee-cli` | Console app (**suggested addition**) | `giclee process <folder>`, `giclee loop <wav>`, `giclee export sfz`. Headless runs of the pipeline, useful for tests, CI, batch reprocessing and tuning |
| `giclee_tests` | Catch2 test runner | DSP unit tests on synthetic signals, plus engine integration tests |

### 1.3 WebView UI and the bridge

- Frontend: TypeScript built with Vite (see open question Q2 for framework choice). In release builds it is bundled into the binary with `juce_add_binary_data` and served through `WebBrowserComponent::Options::withResourceProvider`. In debug builds it loads from the Vite dev server, so UI changes hot-reload without rebuilding C++.
- **Commands** (JS → C++) use JUCE native functions, which return Promises on the JS side. They are wrapped in a small typed RPC layer: `call("session.start", {...})`.
- **Events** (C++ → JS) carry state changes and progress through `emitEventIfBrowserIsVisible`. To avoid flooding the message thread, C++ batches events and sends them at about 30 Hz.
- **Bulk data** (waveform peaks, spectra, sample audio) is fetched over the resource provider as binary URLs, e.g. `/api/peaks/<sampleId>?from=…&to=…&px=…`, not passed as JSON. This keeps a 10-minute free-recording take cheap to draw.
- Plugin parameters in the Player use the JUCE web relays (`WebSliderRelay` and the toggle/combo relays) so host automation and the UI stay in sync.
- A shared `ui/` package holds the components both apps use: waveform canvas, knobs, and the note × velocity grid.
- Windows uses WebView2, with the loader statically linked and the runtime availability checked at startup. macOS uses WKWebView.

### 1.4 Threading and real-time rules

| Thread | Does | Must not |
|---|---|---|
| Audio callback | Read input, push to a lock-free SPSC FIFO; send scheduled MIDI; run audition/player voices | Allocate, lock, do IO, log, or touch `std::string`/JSON |
| Disk writer | Drain the FIFO, write WAV incrementally (JUCE `ThreadedWriter`-style), finalize file | — |
| Pipeline workers (pool) | Run analysis, loop search and sidecar writes for each finished sample | Block the audio thread |
| Message thread | UI bridge, session state, user commands | Run long analysis |
| Offline render thread | Hosted-plugin faster-than-realtime rendering | — |

Communication with the audio thread goes only through atomics, SPSC FIFOs, and pre-allocated command queues. Player voice pools, sample-start caches and streaming buffers are allocated in `prepareToPlay`. When a new sample set loads, it is built off-thread and swapped in atomically; the old set is freed on a background thread. CI runs a debug-only `RealtimeGuard` that asserts if the audio thread allocates (by hooking the allocator in the test runner).

### 1.5 Sessions, persistence, resume

A sampling session is a folder:

```
MySynth_Pad/
  giclee-session.json     # settings, profile, source, plan, per-slot status (journal)
  samples/
    MySynth_Pad_C3_v2_rr1.wav       # smpl chunk: root key, tune, loop points
  analysis/
    MySynth_Pad_C3_v2_rr1.json      # optional per-sample detail (peaks cache etc.)
  MySynth_Pad.sfz
  MySynth_Pad.giclee.json  # set-level sidecar: pitch, cents, loudness, layer, loop scores, confidence
```

- The journal is rewritten atomically (write a temp file, then rename) after every slot changes state, so a crash or power cut loses at most the note being recorded at that moment.
- **Resume:** open the session, recompute which slots are missing or failed, and continue. In manual mode this same mechanism handles the "fill the gaps across several sessions" workflow.
- After each sample finishes processing, the SFZ and sidecar are regenerated, so the set is playable while sampling is still in progress.

### 1.6 Capture sources

```cpp
class CaptureSource {           // giclee_engine
  virtual void prepare(const CaptureFormat&) = 0;
  virtual bool supportsOffline() const = 0;
  virtual void trigger(const SlotRequest&) = 0;   // note on / prompt user
  virtual void release(const SlotRequest&) = 0;   // note off
  ...
};
// MidiHardwareSource, HostedPluginSource, ManualSource
```

A single `SamplingScheduler` turns settings into a list of slots (note × layer × RR), drives the source, and passes the captured audio to the pipeline. Manual mode uses the same scheduler and the same slots, but a human performs the trigger.

### 1.7 Processing pipeline

These are the per-sample stages from the brief. Each is a pure function in `giclee_dsp`, configured by a `ProcessingParams` struct that comes from the profile:

1. Onset detection and trim. Manual mode keeps the pre-roll.
2. RMS envelope, then segmentation into attack, decay, sustain and release.
3. One-shot or looped decision.
4. Pitch: an expected period from MIDI, checked with MPM. Manual mode uses MPM on the sustain portion, with YIN as a fallback and octave-error correction.
5. Loop search: fix the loop end, search start positions with normalized cross-correlation over a window scaled to the pitch period, scoring stereo jointly. A bonus favours whole-period lengths and a soft weight favours zero crossings. The search runs coarse to fine, with the coarse pass on a decimated envelope-normalised signal and an FFT-accelerated NCC. It keeps the top K candidates.
6. Crossfade parameters (equal-power or linear), stored as metadata.
7. Quality score (seam discontinuity + spectral-flux jump) with the fallback chain: longer loop with a longer crossfade, then no loop with the long recording kept.
8. Normalization (off / per sample / per set). Manual mode defaults to per set.

Every result carries a confidence value, and the review UI uses those values to flag samples.

### 1.8 Export

**SFZ is the master format.** The `.giclee.json` sidecar holds whatever SFZ can't express: analysis results, confidence values, layer loudness bands, the velocity curve that was used, loop candidates, and per-sample flags. Every exporter reads a `SampleSet` that is loaded from SFZ + sidecar, so any saved set (and any basic third-party SFZ, using defaults where the sidecar is missing) can be exported again later without re-sampling.

```cpp
struct FormatCapabilities {          // what a target can represent
  bool loopCrossfade, roundRobin, randomRR, velocityCrossfade,
       perZoneTune, releaseTrigger, filterEnvelope;
  int  maxZones, maxVelocityLayers, maxRoundRobins;   // 0 = unlimited
  std::vector<int> sampleRates; std::vector<int> bitDepths; // empty = any
};
class Exporter {
  virtual std::string id() const = 0;                 // "decentsampler", ...
  virtual FormatCapabilities capabilities() const = 0;
  virtual ExportResult run(const SampleSet&, const ExportOptions&,
                           const std::filesystem::path& outDir) = 0;
};
// ExportResult { files written, std::vector<ExportWarning> warnings }
```

- There is one class per format, all registered in an `ExporterRegistry`. The UI lists exporters with checkboxes, and several can run in a single pass.
- **Warnings come from two places.** A generic pass compares the set against `capabilities()`, e.g. "Elektron: 4 round robins collapsed to 1", "MPC: loop crossfade not supported, bake crossfades?". Each exporter can add its own warnings on top. Warnings are shown *before* export (preflight) and saved in an `export-report.txt` next to the output.
- **Sample conversion:** if a target needs a particular sample rate or bit depth, the exporter writes converted copies (high-quality resampler and TPDF dither in `giclee_dsp`) and never touches the master WAVs.
- **The baked-crossfade option** (below) is shared, so any exporter whose `loopCrossfade` is false can use it.

Native exporters:

| # | Format | Output | Notes |
|---|---|---|---|
| 0 | SFZ (master) | `.sfz` + WAVs + sidecar | Always written |
| 1 | Decent Sampler | `.dspreset` XML, optional `.dslibrary` (zip) | Generated UI: ADSR, filter cutoff/resonance, volume, wired to bindings; loop points + crossfade; RR via sequence settings |
| 2 | Bitwig / Studio One | `.multisample` (zip with XML + WAVs) | Zones, key/velocity ranges, root, tune, loops |
| 3 | Akai MPC keygroup | `.xpm` XML + WAVs | Keygroup and layer limits enforced with warnings |
| 4 | Elektron | `.elmulti` + WAVs | Key/velocity zones, loops, Elektron's sample-format requirements |
| — | Kontakt-ready WAVs | WAVs only | e.g. `{name}_{note}_{root}_{lokey}-{hikey}_{lovel}-{hivel}_{rr}.wav`, plus `smpl` root/loop. The README documents the matching Kontakt auto-map token setup |
| — | ConvertWithMoss (external) | whatever it supports (NKI, EXS, Ableton, TAL, …) | See below |

Each format's capability table is checked against its spec or reference files when its exporter is built, and the result is written up in `docs/export-formats.md`.

**ConvertWithMoss bridge:** an optional exporter that runs the user's own ConvertWithMoss installation as an external process (`juce::ChildProcess`) on the SFZ output. It is never bundled or linked. The user sets its path in Preferences, and Giclee detects the version and which target formats it offers, then runs it off the message thread with a timeout. Its output is captured into the export report, and failures appear as warnings rather than crashes. The command-line flags are confirmed against the current ConvertWithMoss release when this is built.

**Tests:**
- Every exporter has a golden-file test: a deterministic synthetic set is exported and the output is compared with checked-in files under `tests/golden/<format>/`. XML is normalized first, zip entries are compared by contents, and WAVs by `smpl`/`inst` chunk fields and a hash of the audio data.
- Formats we can parse (SFZ, dspreset, multisample, xpm, elmulti) also get a round-trip test: export, read back, and check that the zone maps and loop data are equal.
- If ConvertWithMoss is available, an optional CI job also uses it to read our native outputs back in as an independent check.

SFZ opcodes used: `lokey/hikey/pitch_keycenter`, `lovel/hivel`, `tune`, `loop_mode`, `loop_start/loop_end`, `loop_crossfade`, `seq_length/seq_position`, `ampeg_release`, and `xfin_*/xfout_*` for velocity crossfades.

**Something to know:** `loop_crossfade` is an ARIA/sfizz extension, and the WAV `smpl` chunk has no crossfade field. Players that ignore it will click at a seam that isn't already smooth. I suggest an optional **"Bake crossfades for compatibility"** export that writes copies of the samples with the crossfade rendered into the audio, which is the standard trick. It is off by default, so the default output stays non-destructive as the brief asks.

### 1.9 Giclee Player engine

- An SFZ parser covering the subset above plus `<control>`/`<global>`/`<group>`/`<region>`, `#define`, `default_path`, `sample`, `offset`, `volume/pan`, `amp_veltrack`, `trigger=release` (basic), and `ampeg_*`. Unknown opcodes are logged and ignored.
- Voice playback with cubic or 4-point Hermite interpolation (sinc as an optional quality setting), the loop crossfade applied at playback, round-robin cycling via `seq_*` (with random RR as an option), and velocity crossfades.
- Amp ADSR, a TPT state-variable filter (LP/BP/HP) with its own envelope, volume, pan and tune.
- Streaming: preload the first N frames of every sample (and the whole loop region of looped samples), with a disk thread filling per-voice ring buffers. The `SampleSource` interface is in place from the first Player phase, but the first implementation loads everything into memory. Streaming is added later behind the same interface.
- A polyphony limit and voice stealing (oldest released voice first, then quietest, then oldest), with a short fade on stolen voices.

### 1.10 Hosted plugins (VST3/AU)

- Scan with `KnownPluginList`, using a separate scanner process so a plugin that crashes during scanning doesn't take down the app.
- The plugin is loaded in-process for v1. The user opens the plugin's own editor to choose a preset; Giclee saves the plugin state (`getStateInformation`) into the session so resume uses identical sound. Out-of-process hosting can come later if crashes turn out to be a real problem.
- Offline rendering: `setNonRealtime(true)`, process in blocks as fast as possible, send note-off after the hold time and render the tail until it falls below the noise threshold. Some sample-streaming plugins (Kontakt-style) don't render correctly offline, so a "Render in real time" toggle is available, with automatic detection via a quick test render compared against a real-time render.
- Hosted mode has no latency compensation beyond the plugin's reported latency. Optional "warm-up" notes deal with plugins whose first note sounds different.

### 1.11 Hardware specifics

- **Latency:** play a test note (or a percussive patch if the user chooses), find the onset, and repeat about 5 times, taking the median. There's also an optional **loopback ping** (output-to-input cable) that measures pure interface latency separately from the synth's own attack delay. Compensation is reported as two figures: interface latency and instrument response.
- A noise-floor check (a few seconds of silence, giving RMS and peak) feeds the trim and release thresholds and warns when the floor is too high.
- A gain check with a clip warning: a live meter, plus flagging for any sample with peaks ≥ −0.3 dBFS, with an option to re-record clipped samples.

---

## 2. Repository layout

```
giclee/
  CMakeLists.txt  CMakePresets.json  cmake/ (CPM, JUCE pinning, warnings, sanitizers)
  libs/dsp/        include/giclee/dsp/*.h  src/*.cpp           # pure C++
  libs/engine/     include/giclee/engine/*.h  src/*.cpp        # JUCE, no GUI
  apps/sampler/    Source/ (Main, MainWindow, AppController, Bridge)
  apps/player/     Source/ (PluginProcessor, PluginEditor, Bridge)
  apps/cli/
  ui/              package.json vite.config.ts
    shared/        (bridge client, waveform canvas, grid, controls)
    sampler/       player/
  profiles/        *.json (starter profiles, installed as factory presets)
  tests/           dsp/ engine/ fixtures/ signals/ (synthetic generators)
  docs/            architecture.md, sfz-support.md, profile-format.md
  .github/workflows/build.yml
  README.md LICENSE .gitignore .clang-format PLAN.md
```

Dependencies are fetched with CPM.cmake and pinned to exact versions: JUCE 9 (exact tag pinned in Phase 1 after checking that release's WebView API), Catch2 v3, nlohmann/json (or JUCE's `var`/`JSON` inside the engine; `giclee_dsp` doesn't need JSON), and PFFFT or KissFFT for FFT inside `giclee_dsp` so it stays JUCE-free.

Toolchain: C++20, macOS 11+ universal binary (arm64 + x86_64), Windows 10+ with MSVC 2022.

---

## 3. Changes to the brief (please confirm or reject)

1. **CI from Phase 1, not Phase 9.** It's much cheaper to catch Windows/WebView2 build breakage on day one.
2. **Data model, WAV/SFZ IO and the synthetic-signal library come before hardware.** They're needed by everything else and can be tested without any devices.
3. **Hosted VST3/AU moves up to straight after the DSP pipeline, ahead of hardware.** Hosting is deterministic, works without hardware, and gives a real end-to-end test (render → process → SFZ → Player) using a tiny built-in test synth. Hardware then only adds device IO and latency on top of a proven scheduler.
4. **The review UI (grid + waveform editor) comes before manual mode.** Manual mode reuses both the progress grid and the waveform/slice editor, so building them first avoids doing the work twice.
5. **Two DSP/engine libraries instead of one core** (§1.1).
6. **`giclee-cli`** as a small extra target (§1.2).
7. **An optional baked-crossfade export** for compatibility with other players (§1.8).
8. **Streaming in the Player:** the interface ships in the first Player phase, the implementation in the final phase (as in the brief).
9. **The export phase sits right after the Player phase, as requested.** Exporters only need the `SampleSet` model, so they're built and golden-tested on synthetic sets with synthetic loop points. Once the analysis phase lands, they automatically pick up real loop data.

---

## 4. Phases

Each phase ends with something that builds in CI and can be run or tested.

### Phase 1 — Scaffold, CI, app shells, bridge
- [x] Repo: README, LICENSE (GPLv3), .gitignore, .clang-format, CMakePresets; `git init`
- [x] CMake + CPM; JUCE 9.0.4 pinned (hash-checked); `giclee_dsp`, `giclee_engine` stub libraries
- [x] `GicleeSampler` app window hosting a WebView; `GicleePlayer` VST3/AU/Standalone with a WebView editor (the editor hasn't been opened in a DAW yet)
- [x] Svelte/TS/Vite frontend; dev-server mode (Debug, `GICLEE_UI_DEV_URL`) plus embedded resources; mock backend for browser-only UI work
- [x] Typed bridge: `app.ping` round-trip, `app.heartbeat` event, `/api/demo/peaks` binary endpoint drawn by `Waveform.svelte`; Player volume parameter through `WebSliderRelay`
- [x] Catch2 tests: DSP level/peaks; bridge RPC router, event hub, resource router (24 tests)
- [x] GitHub Actions: macOS (universal) and Windows builds plus tests; build artifacts uploaded. First run green (run 40231296320, about 10 min on macOS and 7.5 min on Windows with a cold cache)
- **Done when:** CI is green on both OSes, and both apps open and show a UI that talks to C++.

### Phase 2 — Data model, file IO, synthetic signals
- [ ] `SampleSet` / `Slot` / `SampleInfo` model; session journal read/write with atomic save
- [ ] WAV writer/reader with `smpl` chunk (root key, pitch fraction, loops) and `inst` chunk
- [ ] `Exporter` interface; `SfzExporter`; `GicleeJsonExporter` (sidecar schema v1, documented)
- [ ] Naming-pattern engine (`{name}_{note}_{vel}_{rr}` and related tokens, note-name styles C3/C4)
- [ ] Velocity-curve module (linear/exp/log/custom points) → layer velocity ranges
- [ ] Synthetic signal library: sine, saw, detuned saws (beating), slow LFO, decaying pluck, noise, cents-offset tones, weak-fundamental tones, a long multi-note take with varying loudness
- [ ] `giclee-cli` skeleton able to generate a synthetic test set and export SFZ
- **Done when:** the CLI produces a synthetic multisample set that loads correctly in sfizz.

### Phase 3 — Giclee Player, basic playback
- [ ] SFZ parser (subset in §1.9) with tests
- [ ] Voice engine: zone lookup, velocity crossfades, RR, loops with crossfade, release, tune
- [ ] Amp ADSR, SVF filter with envelope, volume/pan/tune parameters
- [ ] Polyphony and voice stealing; RT-safety assertions in tests
- [ ] `SampleSource` interface (in-memory implementation now)
- [ ] Player UI: load set, keyboard with zone display, parameters via relays; state saved with the project
- **Done when:** the Player plays the Phase 2 synthetic set in a DAW on both OSes.

### Phase 4 — Export formats
- [ ] `FormatCapabilities`, `ExporterRegistry`, preflight warning pass, export report
- [ ] Sample conversion for exports (resample + dither) without touching the masters
- [ ] Shared baked-crossfade utility
- [ ] Decent Sampler: `.dspreset` with generated UI (ADSR, cutoff/resonance, volume), loops/crossfade, RR; optional `.dslibrary`
- [ ] Bitwig/Studio One `.multisample`
- [ ] Akai MPC keygroup `.xpm`
- [ ] Elektron `.elmulti`
- [ ] Kontakt-ready WAVs (filename tokens + `smpl`), with auto-map instructions in the docs
- [ ] ConvertWithMoss external-process exporter (path setting, version/format detection, timeout, log capture)
- [ ] Golden-file tests for every format; round-trip tests where we have a parser
- [ ] `docs/export-formats.md` capability and warning table; export dialog in the Sampler UI; `giclee export --format …` in the CLI
- **Done when:** the synthetic set exports to all four native formats and loads in each target (checked manually once per format), and the golden tests pass in CI.

### Phase 5 — Analysis, pitch, loop finder (+ tests)
- [ ] Onset detection (energy + spectral flux), trim with pre-roll
- [ ] RMS envelope + ADSR segmentation; one-shot decision
- [ ] Pitch: MPM and YIN, sustain-only analysis, octave-error handling, cents offset, confidence
- [ ] Loudness: peak, short-term RMS of the attack, spectral centroid (brightness)
- [ ] Loop finder (§1.7 step 5), top-K candidates; crossfade; quality score; fallback chain
- [ ] Normalization modes
- [ ] Pipeline runner on a worker pool; `giclee process` in the CLI
- [ ] Tests for every synthetic signal, with explicit tolerances (e.g. pitch within ±2 cents for clean tones; loop-seam score thresholds; the pluck is classified as one-shot; noise is classified as unpitched)
- **Done when:** the test suite passes, and the CLI processes a folder of WAVs into a looped SFZ set.

### Phase 6 — Sampling engine + hosted VST3/AU
- [ ] `CaptureSource` interface; `SamplingScheduler` (slot plan, hold/release/gap)
- [ ] Disk writer from a lock-free FIFO; process each sample as soon as it's written; SFZ refreshed live
- [ ] Pause / resume / cancel; resume an interrupted session
- [ ] Plugin scanning (out-of-process), loading, editor window, state capture
- [ ] Offline faster-than-realtime render, plus a real-time fallback toggle
- [ ] Built-in test synth for deterministic end-to-end tests in CI
- [ ] Sampler UI: source picker, sampling settings form, live progress
- **Done when:** sampling a hosted synth produces a playable set; the end-to-end test passes in CI.

### Phase 7 — Hardware sampling (MIDI out + audio in)
- [ ] Audio and MIDI device selection (channel, input channels, mono/stereo, SR/bit depth)
- [ ] Sample-accurate MIDI scheduling relative to the recorded stream
- [ ] Latency measurement (test-note median, optional loopback ping) + compensation
- [ ] Noise-floor check; input meter; clip detection and re-record option
- **Done when:** a full hardware run works, verified on a real synth or with a loopback stand-in.

### Phase 8 — Profiles + auto mode
- [ ] Profile JSON schema (documented, versioned); the seven starter profiles; save/duplicate custom profiles
- [ ] Classifier for the test note: decay, pitch stability, spectral movement, beating, pitched/unpitched
- [ ] Profile suggestion with an explanation; accept/change UI
- [ ] Classifier tests on synthetic signals
- **Done when:** auto mode suggests the expected profile for each synthetic class.

### Phase 9 — Review UI
- [ ] Notes × layers × RR grid with statuses (good/fair/flagged/one-shot/missing) and filters
- [ ] Waveform canvas (zoomable, peaks from C++), draggable loop start/end and crossfade length
- [ ] Audition while editing (loop parameters pushed to the audition voice through atomics); snap to candidates
- [ ] Re-run loop search on a single sample or the whole set with different settings
- [ ] Quick-fix actions: relabel note, reassign layer, retake, change key range
- **Done when:** a set can be fully reviewed and fixed from the keyboard and mouse.

### Phase 10 — Manual mode: guided capture
- [ ] Noise floor + calibration (softest/loudest) → layer loudness bands
- [ ] Big prompt view; arm → onset → end-of-note detection (decay/release/max length) → advance
- [ ] Controls: retake/skip/back/accept; spacebar, on-screen, MIDI remote (learnable), auto-advance
- [ ] Count-in/metronome on a separate output
- [ ] Pitch mismatch and wrong-layer warnings with keep/relabel/retake
- [ ] Unpitched mode: manual key assignment or sequential mapping
- [ ] Progress grid (reused from Phase 9), multi-session gap filling
- **Done when:** a whole guided set can be captured using the keyboard alone.

### Phase 11 — Manual mode: free recording + auto-slice
- [ ] Long-take recording to disk
- [ ] Slicing with onset detection + silence gaps, plus a sensitivity control
- [ ] Slice editor on the waveform: add/remove/move split points
- [ ] Automatic note identification and layer sorting with user reassignment
- [ ] Tests using the synthetic long take
- **Done when:** a single long take becomes a mapped, layered set after review.

### Phase 12 — Streaming, polish, release
- [ ] Player disk streaming behind `SampleSource`; preload-size setting; load tests with large sets
- [ ] Performance pass (loop search speed, UI on large sets)
- [ ] Error handling and UX polish; docs (user guide, SFZ support table)
- [ ] Release workflow: tagged builds, installers/zips; signing/notarization (pending Q6)
- **Done when:** v0.1.0 is released.

---

## 5. Decisions (resolved 2026-10-25)

| # | Topic | Decision |
|---|---|---|
| 1 | License | **GPLv3** (checked against JUCE 9's open-source licence when pinning) |
| 2 | Frontend | **Svelte + TypeScript + Vite**; waveform drawn on a hand-written canvas |
| 3 | Phase order | **Approved** as listed in §4 |
| 4 | Player engine | **Our own engine**; sfizz used only as a compatibility reference in tests |
| 5 | Plugin formats | VST3 + AU + Standalone. **CLAP** is added if JUCE 9 supports it natively (in Phase 1), otherwise via clap-juce-extensions in Phase 12. AAX and Linux are out of scope for now |
| 6 | Signing | **Unsigned** CI builds for now; signing and notarization deferred |
| 7 | Identifiers | Company: **Adam Waito**. Bundle IDs: **`com.adamwaito.giclee.sampler`**, **`com.adamwaito.giclee.player`**. Manufacturer code: **`AdWa`**. Player plugin code: **`GcPl`**. These are frozen once released, because changing them breaks DAW sessions that use the plugin. Apple's AU rules require at least one uppercase letter, which both codes have |
| 8 | Defaults | 48 kHz / 24-bit / stereo; `{name}_{note}_{vel}_{rr}`; C3 = MIDI 60 (configurable) |
| 9 | Export verification | Match against published spec/reference files; you check on real targets when you can |

### Original questions (kept for context)

1. **License: GPL or MIT?** JUCE 8 moved its open-source licence to AGPLv3 (check this for JUCE 9 when pinning). Binaries that link JUCE must therefore either be AGPL-compatible or be built under a JUCE commercial or personal licence. The VST3 SDK is MIT as of 3.8.
   - *GPLv3* (or AGPLv3) is the simplest consistent choice for a JUCE-based app.
   - *MIT* also works for our source code, but the distributed binaries would still carry JUCE's terms. That is legal, just less tidy to explain.
   - **My lean: GPLv3.** It's the most common choice for open-source JUCE projects.
2. **Frontend stack:** vanilla TypeScript, Preact, or Svelte? I lean towards **Svelte** (or Preact) with TypeScript: small bundles, reactive state that suits the grid and progress views, and the waveform drawn on a hand-written canvas either way.
3. **Phase reordering** (§3): OK to move hosted plugins and the review UI earlier, as proposed?
4. **Player engine:** should we write our own, as planned, or embed **sfizz** (BSD-2, mature, with streaming)? sfizz would give broader third-party SFZ support for free. Our own engine is simpler, fully under our control, and shared with the Sampler's audition, so the loops sound identical. **I recommend our own engine**, with sfizz as the reference for compatibility tests.
5. **Plugin formats:** VST3 + AU + Standalone as in the brief. Do you also want **CLAP** (natively, if JUCE 9 supports it, otherwise clap-juce-extensions)? Is AAX and Linux out of scope for now?
6. **Releases:** do you have an Apple Developer ID and a Windows code-signing certificate, or should CI just produce unsigned builds for now?
7. **Identifiers:** company/manufacturer name, bundle ID prefix (e.g. `com.giclee`), and a 4-character manufacturer code for the plugin.
8. **Defaults:** recording defaults of 48 kHz / 24-bit / stereo, and a default naming pattern of `{name}_{note}_{vel}_{rr}` with C3 = MIDI 60 (Yamaha/Ableton style; Roland/SFZ-spec style is C4 = 60). Are those fine?
9. **Export targets you can test with:** do you have an MPC (and which firmware or MPC software version), an Elektron device that loads `.elmulti`, Bitwig or Studio One, Decent Sampler, and Kontakt? Golden tests only prove that we're consistent with ourselves; the first export of each format needs checking on the real target. Whatever you can't check yourself, I'll match against published reference files.

---

## 6. Log

- 2026-10-25 — Plan drafted. Waiting on approval and answers to §5.
- 2026-10-25 — Phase 1 implemented locally: JUCE 9.0.4, Svelte 5.57 / Vite 8.3 / TS 6.0 (svelte-check doesn't support TS 7 yet), Catch2 3.16.1, CPM 0.43.2. Waiting for a GitHub push to verify CI.
- 2026-10-25 — All questions resolved; see §5 Decisions.
- 2026-10-25 — Brief revised: the "Output" section's last bullet is replaced by the "Export formats" section (SFZ master; Decent Sampler, Bitwig/Studio One, MPC, Elektron exporters; Kontakt-ready WAVs; ConvertWithMoss bridge; per-format warnings). New Phase 4 added; later phases renumbered 5–12.
