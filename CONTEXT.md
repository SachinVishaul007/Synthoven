# Synthoven / Chop Sample Browser - Developer Context & Next Steps

This document provides a complete summary of the codebase, recent implementations, architecture, and the detailed implementation plan for the upcoming session. Use this file to seed a new chat session.

---

## 1. Project Vision & Architecture
The **Chop Sample Browser** is a premium JUCE-based audio plugin (VST3 and Standalone) designed to manage, preview, and generate sample libraries.

### Core Architecture
- **Audio Engine**: One-shot audition player implemented in `PluginProcessor.cpp` via `juce::AudioTransportSource` feeding incoming buffers in `processBlock`.
- **Search System**:
  - *Classic keyword search*: Matches filename substrings.
  - *Local AI Semantic Search*: Generates a 512-dimension audio embedding during indexing and matches it against natural language query embeddings generated via the `text_encoder.onnx` model.
- **Background Scanner**: Runs a `LibraryScannerThread` recursively parsing the target directory and indexing sample files.
- **Frontend/UI (C++ JUCE)**: Left sidebar for navigation (collapsible accordion tabs), center area for sample list details, right side for AI Stable Audio generation settings, and bottom for playback visualizer.

---

## 2. Work Completed So Far

### A. Sidebar & Layout Restructuring
- Restructured `PluginEditor.cpp` and `PluginEditor.h` to arrange left sidebar tab buttons vertically (`MY SOUNDS`, `GENERATED`, `FAVORITES`, `SETTINGS`) like an accordion.
- Pinned `selectFolderButton`, `refreshButton`, and `fileTree` to expand dynamically under the "My Sounds" tab, while collapsing for the other tabs.
- Moved `generatedList` and `favoritesList` to the center pane next to the main search results `list` to occupy full width for premium layout spacing.

### B. Interactive Settings Panel
- Replaced the settings label placeholder with a custom `SettingsPopupComponent` launched in a `juce::CallOutBox` next to the `SETTINGS` button.
- Exposes:
  - Text editor to view and change the backend API Base URL (updates the `ChopApiClient` dynamically).
  - Directory chooser link to configure the library folder.
  - Scanned file count and AI indexing progress stats.

### C. Generated Samples Indexing & Favorites
- Fixed Stable Audio generation callbacks to automatically trigger a library scan, index the generated sample in the background database, and refresh the `generatedList` without switching the user's selected tab.
- Integrated favorite toggling (heart icon) referencing original files directly on disk using a JSON-based database (`favorites.json` in the user's Music directory).

---

## 3. Current Implementation Plan (Next Steps)

For the next session, implement the following UI enhancements and playback controls:

### Task Checklist

#### [ ] Tiny Waveform Previews in Sample Rows
- Increase list item row height from `44` to `56` in `SampleListBox.cpp` and `SampleListBox.h`.
- Make sample name text larger (`14.0f` Bold) and display mood tags as small rounded background bubble chips.
- Add a mini-waveform preview (width `70px`, height `20px`) next to the heart button, rendering a deterministic envelope waveform shape based on the sample properties/name hash to keep scrolling lag-free.

#### [ ] Static Colorful Waveform Preview & Scrubbing
- Add `WaveformPreviewComponent` to replace the scrolling `juce::AudioVisualiserComponent` at the bottom.
- Link it with `juce::AudioThumbnail` to draw a colorful static waveform of the entire auditioned sample.
- Draw a white vertical playhead line indicating the current position.
- Add scrubbing support: clicking/dragging on the waveform updates `transport.setPosition()`.

#### [ ] Transport Controls & Metadata Info
- Implement Play (▶) and Pause (⏸) symbol buttons in `WaveformPreviewComponent`.
- Display a detailed metadata string (Format, Sample Rate, Mono/Stereo, Duration) next to the wave.
- Remove the legacy `stopButton` next to the search box.

#### [ ] C++ API Adjustments
- Expose public transport and playback methods from `ChopAudioProcessor` (`startPlayback`, `pausePlayback`, `getPlaybackPosition`, `getPlaybackLength`, `setPlaybackPosition`).
- Expose the `formatManager` to allow the waveform preview component to parse audio headers.

---

## 4. How to Resume Work in the New Session
1. **Load this CONTEXT.md** first to guide the agent.
2. The code builds successfully with CMake.
3. Start by adding the public getters/playback methods in `PluginProcessor.h` / `PluginProcessor.cpp`, then create the new `WaveformPreviewComponent` class, and finally update `SampleListBox.cpp` and `PluginEditor.cpp`.
