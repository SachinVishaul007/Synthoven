# Synthoven / Chop Sample Browser - Developer Handoff

Welcome to the project! This document outlines the overall vision, current progress, architectural decisions, and the roadmap for completing the next phases of development for the **Chop Sample Browser** audio plugin.

---

## 1. Overall Vision & Goals
The **Chop Sample Browser** is a modern, premium audio plugin (VST3 and Standalone) designed to streamline sample management for music producers. 

### Key Objectives
* **Instant Direct Search**: Traditional filename and folder search (already implemented).
* **On-Device Semantic Search (ML)**: Enabling natural language queries (e.g., searching for "sad", "dark and ambient pads", "aggressive drums") using Hugging Face's CLAP model running locally on the user's CPU via **ONNX Runtime**.
* **Modern Premium UI**: Clean, responsive, dark-mode design with smooth hover transitions, tag-filtering chips, and a dynamic progress bar for scanning.

---

## 2. Completed Architecture & Setup

### A. Environment & Build System (`CMakeLists.txt`)
* **JUCE 8 Integration**: Built as a standard VST3 and Standalone plugin using CMake.
* **ONNX Runtime Windows x64 ZIP SDK (v1.18.0)**: Automatically fetched, extracted, and linked statically using CMake's `FetchContent`.
* **Post-Build Dynamic Library Packaging**: The CMake script automatically copies `onnxruntime.dll` into the output target directories next to `Chop Sample Browser.vst3` and `Chop Sample Browser.exe` so the binaries run out of the box.
* **Target Formats**: VST3 and Standalone. *(Note: CLAP was removed from the native target list as native JUCE 8 CMake requires the external `clap-juce-extensions` wrapper subproject to build a valid `.clap` binary bundle. This wrapper can be added later if needed).*

### B. User Interface & Indexing Status (`PluginEditor.cpp`/`PluginProcessor.cpp`)
* Added a **"Refresh"** button next to the "Select Folder..." button.
* Built a background `LibraryScannerThread` that recursively indexes the target directory.
* The GUI thread polls progress and displays status dynamically in the bottom bar (e.g., `Indexing library... (X files found)` and updates to completion text).

### C. The ML Preprocessing Pipeline (Phase 2 Completed)
We have fully implemented and compiled the preprocessing steps required by the CLAP transformer:
1. **RoBERTa / GPT-2 BPE Tokenizer** (`Source/BpeTokenizer.h`, `Source/BpeTokenizer.cpp`):
   * Pure C++ tokenizer utilizing JUCE's string conversion helpers (to avoid deprecated `<codecvt>` warnings) and JUCE's JSON parser (zero external dependency).
   * Implements standard byte-level BPE vocabulary mapping and pads/truncates inputs to the shape required by CLAP's text encoder.
2. **DSP Mel-Spectrogram Extractor** (`Source/MelSpectrogramExtractor.h`, `Source/MelSpectrogramExtractor.cpp`):
   * Uses `juce::dsp::FFT` for spectral analysis.
   * Automatically resamples files of any format/sample rate to **48,000 Hz** (linear resampler).
   * Extracts log-mel spectrogram frames using standard parameters: 64 mel bins, Hann window of size 1024, and a hop size of 480 samples.
   * Implements **Slaney normalization** on the triangular mel filterbank to match Python's `librosa` / Hugging Face preprocessing exactly.
   * Handles audio padding (looping short files, cropping long files) to output a flat vector representing exactly **1001 frames x 64 mel-bins** (10 seconds of audio) as expected by the CLAP audio encoder.

---

## 3. Next Steps: Phase 3 & Phase 4

### Phase 3: ONNX Inference & Caching
The next agent should implement ONNX Runtime inference to generate embedding vectors:
1. **Model Files Setup**:
   * Have the plugin look in the standard app data directory (e.g., `%APPDATA%/ChopSampleBrowser/models/`) for the following files:
     * `vocab.json`
     * `merges.txt`
     * `text_encoder.onnx`
     * `audio_encoder.onnx`
2. **Tokenizer & Model Initialization**:
   * Load the tokenizer vocab and merges files in `PluginProcessor` on startup.
   * Initialize two `Ort::Session` objects (one for text encoder, one for audio encoder) using ONNX Runtime C++ APIs.
3. **Index-Time Audio Embeddings**:
   * During library scanning, run the audio files through `MelSpectrogramExtractor` to get the `1001 x 64` spectrogram features.
   * Pass the features to `audio_encoder.onnx` to get a 512-dimension float vector (audio embedding).
   * **Cache Database**: Write these 512-D vectors to a flat binary file (`tag_cache.bin` or inside the XML database) next to `%APPDATA%/ChopSampleBrowser/tag_cache.xml` to avoid recalculating embeddings on subsequent launches.
4. **Query-Time Text Embeddings**:
   * Tokenize user text queries using `BpeTokenizer` to generate `input_ids` and `attention_mask` (pad to length 77).
   * Pass them to `text_encoder.onnx` to get a 512-dimension float vector (text embedding).
5. **Cosine Similarity Match**:
   * Compute the dot product between the text embedding vector and all cached audio embedding vectors.
   * Sort the search results in descending order of similarity score.

### Phase 4: Search UI & Mood Tags
1. **Mood Tag Chips**:
   * Add clickable tag chips (e.g., `[Sad]`, `[Dark]`, `[Happy]`, `[Aggressive]`) above the search bar in `PluginEditor`.
   * Clicking a tag enters it into the search query or automatically triggers a semantic similarity search using that tag as the query text.
2. **Search Toggle**:
   * Allow users to toggle between standard filename/tag substring matching and ML semantic search.

---

## 4. How to Build & Run
To compile the VST3 and Standalone plugin:
1. Open PowerShell and navigate to the build directory:
   ```powershell
   cd d:\Rohan\Antigravity\Synthoven_Fresh\plugin\build
   ```
2. Configure CMake:
   ```powershell
   cmake ..
   ```
3. Build the Release configuration:
   ```powershell
   cmake --build . --config Release
   ```
4. Find the built outputs under `ChopPlugin_artefacts/Release/`.

Happy coding!
