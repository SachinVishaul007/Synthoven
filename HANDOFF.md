# Synthoven (Chop) JUCE Plugin: Handoff & Implementation Brief

This document serves as the implementation plan and handoff document for transitioning the **Chop** sample tool into a native **JUCE Audio Plugin (VST3 / Standalone)**.

---

## 🚀 Current Status & Local Workspace Setup

We have successfully configured the workspace for local development:
* **Java 17 (Eclipse Temurin)** is installed and set as the active compiler.
* **Apache Maven 3.9.6** is set up locally.
* **Database (H2)**: The database configuration in `backend/src/main/resources/application.properties` has been switched to use an **embedded H2 database** (`./data/chopdb`) in PostgreSQL compatibility mode. This removes the local PostgreSQL requirement and works out of the box.
* **Backend Dev Server**: Running at `http://localhost:8080/`.
* **Frontend Dev Server**: Running on Vite at `http://localhost:5173/` (properly proxying API requests to `:8080`).

---

## 📦 Project Architecture Overview

The workspace is structured as:
```text
Synthoven_Fresh/
├── backend/       # Spring Boot application (Java 17 + H2)
├── frontend/      # React + Vite web dashboard (Port 5173)
└── HANDOFF.md     # This brief
```

---

## 🎨 JUCE Audio Plugin Implementation Details

The objective is to turn **Chop** into a VST3 / Standalone plugin running inside a DAW on Windows using a **JUCE 8 WebView UI** approach.

### 1. Web UI Wrapper (C++ / JUCE)
* **Components**: 
  * `AudioProcessor`: Main audio/process logic.
  * `AudioProcessorEditor`: Visual wrapper hosting a `juce::WebBrowserComponent` that fills the window.
* **WebView Integration**:
  * Run `cd frontend && npm run build` to generate `frontend/dist`.
  * Bundle `frontend/dist` as a resource inside the JUCE app.
  * Use JUCE 8's WebView resource provider to serve files directly to `juce::WebBrowserComponent`.

### 2. Audio Auditioning Engine (C++)
* **Requirement**: Auditioning must happen through the DAW/plugin audio path, **not** the browser `<audio>` element inside the WebView.
* **Logic**:
  * Triggered via a JS-to-Native bridge callback when a user auditions/plays a sample in the UI.
  * Download/stream the audio bytes in a background thread from `GET http://localhost:8080/api/player/stream/{id}`.
  * Use JUCE's `AudioFormatReader` to parse the downloaded bytes.
  * Resample the audio to the host's active sample rate and play it using `AudioTransportSource` / `PositionableAudioSource`.

### 3. DAW Drag-and-Drop Integration (Crucial Feature)
* **Goal**: Enable producers to drag audio rows directly onto a DAW timeline track.
* **Logic**:
  * Expose a native drag-start function to JS via the WebView bridge.
  * When a user drags a row, the frontend JS triggers the bridge: `window.juce.postMessage({ type: 'startDrag', sampleId: id })`.
  * The C++ side receives the sample ID, fetches/downloads the file to a temporary location (or uses its local library file path if it's a local sample), and calls:
    ```cpp
    juce::DragAndDropContainer::performExternalDragDropOfFiles({ absoluteFilePath }, false);
    ```

### 4. Folder/File Import Integration
* **Adjustment**: Browsers' file/folder pickers inside WebViews do not have full system/DAW integration.
* **Logic**:
  * Replace the "Import folder" action in the UI with a bridge call to C++.
  * C++ displays a native `juce::FileChooser` dialog to let the user select a folder or files.
  * The C++ side then POSTs the files to the `/api/library/upload` endpoint, or registers the watched folder path via `/api/library/folders`.

---

## 🛠️ Build Requirements & Guide

1. **Prerequisites**:
   * JUCE 8 (CMake setup preferred).
   * Visual Studio 2022 (with "Desktop development with C++" workload).
   * Windows WebView2 Runtime installed on the host.
2. **API Endpoint Contract**:
   * Refer to the endpoints listed in the user brief for all search, generation polling, stats, and audio stream fetching APIs.
