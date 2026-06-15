#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "SampleListBox.h"
#include "WaveformPreviewComponent.h"
#include "AudioInputDropTarget.h"

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

    void selectLibraryFolder();

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
    void doGeneratePrompt();      // text-to-audio from the prompt
    void doGenerateVariations();  // audio-to-audio: variations of the dropped source
    // Enables each generate button only when its required input is present
    // (prompt text / source audio) and no generation is in progress.
    void updateGenerateButtons();
    // Sequentially generates `total` results (text- or audio-to-audio), one per
    // run with a fresh backend seed, so each is a distinct variation.
    void generateAudioVariations (const juce::String& prompt, double duration, double cfgScale,
                                  const juce::String& category, const juce::File& initAudio,
                                  int total, int index);
    juce::File getGeneratedFolder() const;
    void loadLocalFolder (const juce::File& folder);
    void setStatus (const juce::String& text);
    void timerCallback() override;

    void setLibraryFolder (const juce::File& folder);

    ChopAudioProcessor& processorRef;

    juce::TextEditor  searchBox;
    juce::TextButton  searchButton  { "Search" };
    WaveformPreviewComponent waveformPreview;
    juce::ToggleButton semanticSearchToggle { "Mood" };

    std::vector<std::unique_ptr<juce::TextButton>> tagButtons;
    std::vector<juce::String> tags = { "Happy", "Dark", "Sad", "Energetic", "Ambient", "Cinematic", "Punchy" };
    void updateTagHighlighting (const juce::String& activeTag);

    juce::ImageComponent logoComponent; // Chop logo shown in the header (replaces text title)
    juce::Label       statusLabel;

    // ── Generation setup panel (right) ─────────────────────────────────────
    juce::Label       genHeaderLabel;
    juce::Label       promptHeaderLabel;
    juce::TextEditor  promptEditor;
    juce::Label       durationHeaderLabel;
    juce::Slider      durationSlider;
    juce::Label       durationValueLabel;
    juce::Label       variationsHeaderLabel;
    juce::ComboBox    variationsCombo;
    juce::Label       audioInputHeaderLabel;
    AudioInputDropTarget audioInputDropTarget;
    juce::TextButton  generatePromptButton     { "GENERATE FROM TEXT" };
    juce::TextButton  generateVariationsButton { "GENERATE FROM AUDIO" };
    bool              isGenerating = false;

    // ── Audio visualizer (waveform of the audio being auditioned) ──────────
    juce::Label       visualizerHeaderLabel;

    juce::TextButton  selectFolderButton { "Select Folder..." };
    juce::TextButton  refreshButton      { "Refresh" };
    FoldersOnlyFilter folderFilter;
    juce::TimeSliceThread fileScannerThread;
    juce::DirectoryContentsList directoryList;
    juce::FileTreeComponent fileTree;

    // ── Sidebar Tabs ────────────────────────────────────────────────────────
    enum class TabMode { MySounds, Generated, Favorites };
    TabMode currentTab = TabMode::MySounds;
    void setTabMode (TabMode newMode);

    juce::TextButton tabMySounds { "MY SOUNDS" };
    juce::TextButton tabGenerated { "GENERATED" };
    juce::TextButton tabFavorites { "FAVORITES" };
    juce::TextButton settingsButton { "SETTINGS" };

    std::unique_ptr<SampleListBox> list;            // General search results list
    std::unique_ptr<SampleListBox> generatedList;   // List for generated tab
    std::unique_ptr<SampleListBox> favoritesList;   // List for favorites tab

    void loadGeneratedSamples();
    void loadFavoriteSamples();
    void showSettingsPopup();

    std::unique_ptr<juce::FileChooser> chooser;

    bool wasScanningLastCheck = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChopAudioProcessorEditor)
};
