# Project Handoff: Synthoven "Chop" Sample Browser Plugin

This document outlines the architecture, vision, recent implementations, and next steps for the Synthoven "Chop" Sample Browser plugin to provide a smooth transition for the next agent/developer.

---

## 1. Project Vision & Goals

The **Chop Sample Browser** is a native C++ JUCE utility designed to run as a **VST3 Generator Plugin** and a **Standalone Application**. 

Its main objectives are:
* **Zero Backend Dependency**: Local-only folder browsing and indexing to keep the utility incredibly fast, lightweight, and offline-compatible.
* **Seamless DAW Integration**: Enable the user to select their local sample libraries, search across files instantly, audition sounds, and drag-and-drop samples natively directly into a DAW playlist or sampler (such as FL Studio).
* **High Aesthetics**: A dark-themed, sleek UI that looks premium and integrates visually with modern DAW workspaces.

---

## 2. Core Architecture & Components

The codebase is built on **JUCE 8** and organized into standard Processor/Editor components:

### A. Audio Engine & Background Scanning (`PluginProcessor.h/cpp`)
* **`ChopAudioProcessor`**: Manages audio device state, file audition playback, settings persistence (remembering the library folder path across sessions), and background tasks.
* **`LibraryScannerThread`**: A nested background `juce::Thread` that recursively traverses the selected `libraryFolder` to index supported audio files (`.wav`, `.mp3`, `.aif`, `.aiff`, `.flac`). It updates atomic variables (`isScanning` and `scannedCount`) in real time to avoid locking the UI thread during deep scans.
* **State Management**: Uses `getStateInformation` and `setStateInformation` to store settings locally in XML configuration.

### B. User Interface (`PluginEditor.h/cpp` & `SampleListBox.cpp`)
* **`ChopAudioProcessorEditor`**: Governs the split layout:
  * **Left Panel**: Contains the "Select Folder..." and "Refresh" buttons, and the `juce::FileTreeComponent` (configured with a folders-only filter).
  * **Right Panel**: Contains the search bar, "Search" and "Stop" buttons, and the sample list.
* **`FoldersOnlyFilter`**: Inherits from `juce::FileFilter`. Overrides `isFileSuitable` to always return `false`. This ensures that `juce::FileTreeComponent` displays folders only, hiding individual audio files from the left tree.
* **`SampleListBox`**: Custom list box component displaying matching samples.
  * Captures row selection to trigger audio playback (auditioning).
  * Implements `beginExternalDragForRow()` using `juce::DragAndDropContainer` to start native OS drag-and-drop actions to external targets (like DAW windows).

---

## 3. Recently Implemented Features

1. **Local-Only Search Indexing**: Removed backend Spring Boot API integrations and category tabs. Search now matches user queries against a local memory index gathered by the background scanner.
2. **Left Panel File Filter**: Filtered out files from the tree view globally (using `FoldersOnlyFilter`), fulfilling the user requirement to only show files on the right side when a specific directory is selected.
3. **Indexing Progress & Status Feedback**: Enabled a `juce::Timer` polling at 100ms. If scanning is active, it updates the bottom status bar: `Indexing library... (X files found)`. Upon completion, it changes to: `Indexing complete. X samples ready.`
4. **Manual Sync/Refresh Flow**: Added a "Refresh" button next to "Select Folder...". It triggers a clean background scan and calls `directoryList.refresh()` to reload tree view directory items.
5. **DAW Drag-and-Drop**: Mapped drag actions from `SampleListBox` to drag the native files directly into external applications.

---

## 4. Development & Build Setup

### Prerequisites
* Windows OS
* CMake (>= 3.15)
* MSVC Compiler / Build Tools

### Compiling
To build the Standalone app and VST3 target, navigate to the `plugin/build` directory and run:
```powershell
cmake --build . --config Release
```

> [!WARNING]
> **Process Lock Warnings**: If the standalone executable (`Chop Sample Browser.exe`) is running, the linker will fail with `LNK1104`. Make sure to close the app or terminate it via terminal:
> ```powershell
> Get-Process -Name "*Chop*" -ErrorAction SilentlyContinue | Stop-Process -Force
> ```

---

## 5. Suggested Next Steps

1. **Waveform Visualizer Component**: Implement a simple visualizer in `SampleListBox` or as a separate section in the editor to show the waveform of the currently selected/auditioned sample.
2. **Fuzzy Search & Filtering**: Enhance `doSearch()` in `PluginEditor.cpp` to support fuzzy string matching (or tokenized search terms) and filters (e.g. searching only for files of a specific length or format).
3. **Category Tagging**: Parse file/folder paths to automatically tag samples (e.g. placing samples in folders containing "Kick" or "Snare" under quick category filters).
