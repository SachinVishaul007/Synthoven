# Chop Sample Browser — JUCE Plugin

A VST3 / Standalone audio plugin that puts the Chop sample tool inside a DAW. It
talks to the existing Chop backend REST API to **search**, **AI-generate**,
**browse**, and **audition** samples, and lets you **drag a sample straight onto
a DAW track**.

This folder is self-contained — it only depends on the running backend (over
HTTP) and on JUCE (downloaded automatically at configure time).

```
plugin/
  CMakeLists.txt        FetchContent pulls JUCE 8; defines the VST3/Standalone target
  Source/
    Sample.h            Backend SampleDto <-> struct mapping + JSON parsing
    ChopApiClient.*     REST client (search, generation poll, library, upload, stream)
    PluginProcessor.*   AudioProcessor + one-shot audition engine (AudioTransportSource)
    PluginEditor.*      Native UI: search, section tabs, results list, transport, import
    SampleListBox.*     Results list; click = audition, drag = external file drag to DAW
  README.md
```

## What it does
- **Search** — POST `/api/search`; shows library hits immediately, polls
  `/api/generation/{jobId}` every 2s and appends AI-generated results.
- **Section tabs** — Library / Favorites / Generated / Recent load the matching
  `/api/library/samples/*` endpoint.
- **Audition** — clicking a row streams `/api/player/stream/{id}` to a temp file
  and plays it through the plugin's output (resampled to the host rate).
- **Drag to DAW** — dragging a row downloads the file (cached) and performs an
  external file drag, so you can drop it onto a track.
- **Import** — a native file picker uploads files via multipart `/api/library/upload`.

## Prerequisites
- **CMake ≥ 3.22** and a C++17 compiler.
  - **macOS:** Xcode + command-line tools.
  - **Windows:** Visual Studio 2022 (Desktop C++ workload).
- **git** (CMake uses it to fetch JUCE).
- The **Chop backend running on `http://localhost:8080`** (see `../backend`).
  Start it with `mvn -DskipTests spring-boot:run`. Generation defaults to the
  `mock` provider, so no API key is needed.

## Build

### macOS
```bash
cd plugin
cmake -B build -G Xcode
cmake --build build --config Release
```

### Windows
```bat
cd plugin
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

### Any platform (Ninja/Make)
```bash
cd plugin
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

First configure downloads JUCE (~a minute). To reuse a local JUCE checkout
instead of downloading, add `-DJUCE_PATH=/path/to/JUCE`.

## Where the plugin lands
`COPY_PLUGIN_AFTER_BUILD` installs the built VST3 to the standard folder:
- **macOS:** `~/Library/Audio/Plug-Ins/VST3/Chop Sample Browser.vst3`
- **Windows:** `%COMMONPROGRAMFILES%\VST3\Chop Sample Browser.vst3`

The **Standalone** build is the quickest way to try it without a DAW — run the
`Chop Sample Browser` app produced under `build/`.

## Configuration
- **Backend URL:** set the `CHOP_API_BASE` environment variable to point at a
  non-default backend (e.g. `CHOP_API_BASE=http://192.168.1.10:8080`). Defaults
  to `http://localhost:8080`.

## Notes & limitations
- The backend must be reachable; this plugin is a client, it does not embed the
  server. (Longer term you could ship the backend as a sidecar or swap Postgres
  for embedded H2 — see the repo README.)
- `mock`-generated samples have no playable file on disk, so auditioning/dragging
  them reports an error. Library/uploaded samples (real files) play and drag fine.
- `registerBasicFormats()` covers WAV/AIFF (and FLAC/Ogg if enabled in the JUCE
  build). For MP3 decoding, enable the relevant JUCE format flag.
- Validate with [`pluginval`](https://github.com/Tracktion/pluginval) before
  loading into a host.
```bash
pluginval --strictness-level 5 --validate "build/ChopPlugin_artefacts/Release/VST3/Chop Sample Browser.vst3"
```
