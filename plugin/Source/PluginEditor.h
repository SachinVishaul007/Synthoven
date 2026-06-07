#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "SampleListBox.h"

class FoldersOnlyFilter : public juce::FileFilter
{
public:
    FoldersOnlyFilter() : juce::FileFilter ("Folders Only") {}

    bool isFileSuitable (const juce::File& file) const override
    {
        return file.isDirectory();
    }

    bool isDirectorySuitable (const juce::File& folder) const override
    {
        juce::ignoreUnused (folder);
        return true;
    }
};

class ChopAudioProcessorEditor : public juce::AudioProcessorEditor,
                                 public juce::FileBrowserListener,
                                 public juce::DragAndDropContainer,
                                 private juce::Timer
{
public:
    explicit ChopAudioProcessorEditor (ChopAudioProcessor&);
    ~ChopAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // ── FileBrowserListener methods ────────────────────────────────────────
    void selectionChanged() override {}
    void fileClicked (const juce::File& file, const juce::MouseEvent& e) override;
    void fileDoubleClicked (const juce::File& file) override {}
    void browserRootChanged (const juce::File& newRoot) override {}

    // ── DragAndDropContainer methods ───────────────────────────────────────
    bool shouldDropFilesWhenDraggedExternally (const juce::DragAndDropTarget::SourceDetails& sourceDetails,
                                               juce::StringArray& files, bool& canMoveFiles) override;

private:
    void doSearch();
    void doGenerate();
    juce::File getGeneratedFolder() const;
    void loadLocalFolder (const juce::File& folder);
    void setStatus (const juce::String& text);
    void timerCallback() override;

    void selectLibraryFolder();
    void setLibraryFolder (const juce::File& folder);

    ChopAudioProcessor& processorRef;

    juce::TextEditor  searchBox;
    juce::TextButton  searchButton  { "Search" };
    juce::TextButton  stopButton    { "Stop" };
    juce::ToggleButton semanticSearchToggle { "AI Search" };

    std::vector<std::unique_ptr<juce::TextButton>> tagButtons;
    std::vector<juce::String> tags = { "Happy", "Dark", "Sad", "Energetic", "Ambient", "Cinematic", "Punchy" };
    void updateTagHighlighting (const juce::String& activeTag);

    juce::Label       titleLabel;
    juce::Label       statusLabel;

    // ── Generation setup panel (right) ─────────────────────────────────────
    juce::Label       genHeaderLabel;
    juce::Label       promptHeaderLabel;
    juce::TextEditor  promptEditor;
    juce::Label       durationHeaderLabel;
    juce::Slider      durationSlider;
    juce::Label       durationValueLabel;
    juce::TextButton  generateSoundButton { "GENERATE SOUND" };

    // ── Audio visualizer (waveform of the audio being auditioned) ──────────
    juce::Label       visualizerHeaderLabel;

    // Local library browser components
    juce::TextButton  selectFolderButton { "Select Folder..." };
    juce::TextButton  refreshButton      { "Refresh" };
    FoldersOnlyFilter folderFilter;
    juce::TimeSliceThread fileScannerThread;
    juce::DirectoryContentsList directoryList;
    juce::FileTreeComponent fileTree;

    std::unique_ptr<SampleListBox>   list;
    std::unique_ptr<juce::FileChooser> chooser;

    bool wasScanningLastCheck = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChopAudioProcessorEditor)
};
