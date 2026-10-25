# Giclee

Giclee is an open-source automatic multisampler and a companion sample player.

- **Giclee Sampler** (standalone app) samples hardware instruments over MIDI, hosted VST3/AU instruments, and non-MIDI acoustic/analog instruments (guided or free recording). It trims, analyses and loops every sample as it's recorded, then exports SFZ and other sampler formats.
- **Giclee Player** (VST3 / AU / standalone) plays the sets Giclee produces, as well as basic third-party SFZ.

> **Status: early development.** Phase 1 (scaffold, CI, app shells and the UI bridge) is in place; nothing samples or plays audio yet. See [PLAN.md](PLAN.md) for the roadmap and current progress.

## Building

Requirements:

| | macOS | Windows |
|---|---|---|
| Compiler | Xcode 15+ (macOS 11+ target) | Visual Studio 2022 (MSVC) |
| CMake | 3.25+ | 3.25+ |
| Node.js | 22+ (builds the web UI) | 22+ |

JUCE 9, Catch2 and (on Windows) the WebView2 SDK are downloaded automatically at configure time. All versions are pinned and their hashes checked.

```sh
cmake --preset release          # or: debug
cmake --build --preset release
ctest --preset release
```

Outputs:

- `build/<preset>/apps/sampler/GicleeSampler_artefacts/<Config>/`
- `build/<preset>/apps/player/GicleePlayer_artefacts/<Config>/` (VST3, AU, Standalone)

Useful options:

| Option | Default | |
|---|---|---|
| `GICLEE_BUILD_UI` | `ON` | Build the web UI with npm. `OFF` embeds a placeholder page, so no Node.js is needed. |
| `GICLEE_BUILD_TESTS` | `ON` | Build the unit tests. |
| `GICLEE_COPY_PLUGINS` | `OFF` | Copy the Player into the system plugin folders after building. |

Set the `CPM_SOURCE_CACHE` environment variable (e.g. `~/.cache/CPM`) to share the JUCE download between build folders.

Release builds are unsigned for now. On macOS you may need to right-click → Open the first time; on Windows, SmartScreen will ask for confirmation. On Windows the UI needs the Microsoft Edge WebView2 Runtime, which ships with Windows 11 and current Windows 10.

## Working on the UI

The UI is Svelte + TypeScript, built with Vite, in [`ui/`](ui). It talks to C++ through a small bridge:

- RPC calls: `call("app.getInfo")`
- Events from C++: `on("app.heartbeat", fn)`
- Binary data (e.g. waveform peaks): `fetchBinary(...)`

The contract is in [`ui/src/shared/api.ts`](ui/src/shared/api.ts).

For hot reload, run the Vite dev server and point a **Debug** build at it:

```sh
cd ui && npm install
npm run dev:sampler          # http://localhost:5173 (dev:player uses 5174)

GICLEE_UI_DEV_URL=http://localhost:5173/ \
  "build/debug/apps/sampler/GicleeSampler_artefacts/Debug/Giclee Sampler.app/Contents/MacOS/Giclee Sampler"
```

Opening the dev server in an ordinary browser also works: calls then go to a mock backend, which is handy for layout work.

## Repository layout

```
libs/dsp      Pure C++20 analysis/DSP (no JUCE). Unit-tested with synthetic signals.
libs/engine   Audio/MIDI, plugin hosting, file IO, playback (JUCE, no GUI).
libs/webui    C++ side of the UI bridge: RPC router, event hub, resource server, WebView.
apps/sampler  Giclee Sampler application.
apps/player   Giclee Player plugin.
ui/           Web frontends for both apps.
tests/        Catch2 tests.
```

## License

Giclee is licensed under the [GNU General Public License v3.0](LICENSE).

Giclee is built on [JUCE](https://juce.com), which is used under the AGPLv3 (JUCE is dual-licensed AGPLv3/commercial). GPLv3 §13 allows combining with AGPLv3 code. Binaries that include JUCE are therefore also subject to the AGPLv3's terms for the JUCE parts. The VST3 SDK is MIT-licensed. *VST is a registered trademark of Steinberg Media Technologies GmbH.*
