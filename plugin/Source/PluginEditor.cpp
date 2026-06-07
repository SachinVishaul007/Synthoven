#include "PluginEditor.h"

ChopAudioProcessorEditor::ChopAudioProcessorEditor (ChopAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      folderFilter(),
      fileScannerThread ("ChopFileScanner"),
      directoryList (&folderFilter, fileScannerThread),
      fileTree (directoryList)
{
    titleLabel.setText ("CHOP", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (20.0f).withStyle ("Bold")));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe0a458));
    addAndMakeVisible (titleLabel);

    searchBox.setTextToShowWhenEmpty ("Search samples, e.g. punchy techno kick",
                                      juce::Colours::white.withAlpha (0.4f));
    searchBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff202024));
    searchBox.onReturnKey = [this] { doSearch(); };
    addAndMakeVisible (searchBox);

    searchButton.onClick = [this] { doSearch(); };
    stopButton.onClick   = [this] { processorRef.stopAudition(); setStatus ("Stopped"); };
    addAndMakeVisible (searchButton);
    addAndMakeVisible (stopButton);

    statusLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.6f));
    statusLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (statusLabel);

    list = std::make_unique<SampleListBox> (processorRef);
    list->onAudition = [this] (const Sample& s)
    {
        if (s.localFilePath.isNotEmpty())
        {
            juce::File f (s.localFilePath);
            if (f.existsAsFile())
            {
                processorRef.auditionFile (f);
                setStatus ("Playing local file: " + s.name);
            }
            return;
        }
    };
    list->onStatus = [this] (const juce::String& s) { setStatus (s); };
    addAndMakeVisible (*list);

    // Setup local library browser components
    selectFolderButton.onClick = [this] { selectLibraryFolder(); };
    selectFolderButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202024));
    selectFolderButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (selectFolderButton);

    refreshButton.onClick = [this] {
        processorRef.triggerLibraryScan();
        directoryList.refresh();
    };
    refreshButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202024));
    refreshButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (refreshButton);

    fileTree.setColour (juce::TreeView::backgroundColourId, juce::Colour (0xff161619));
    fileTree.setColour (juce::TreeView::linesColourId, juce::Colours::white.withAlpha (0.1f));
    fileTree.setColour (juce::TreeView::selectedItemBackgroundColourId, juce::Colour (0xffe0a458).withAlpha (0.2f));
    fileTree.setColour (juce::DirectoryContentsDisplayComponent::textColourId, juce::Colours::white.withAlpha (0.9f));
    fileTree.setColour (juce::DirectoryContentsDisplayComponent::highlightedTextColourId, juce::Colours::white);

    fileTree.addListener (this);
    fileTree.setDragAndDropDescription ("ChopLocalFile");
    addAndMakeVisible (fileTree);

    fileScannerThread.startThread();

    // Load directory from processor or default to Music folder
    auto savedFolder = processorRef.getLibraryFolder();
    if (savedFolder.exists() && savedFolder.isDirectory())
    {
        directoryList.setDirectory (savedFolder, true, false);
        loadLocalFolder (savedFolder);
        processorRef.triggerLibraryScan();
    }
    else
    {
        auto defaultDir = juce::File::getSpecialLocation (juce::File::userMusicDirectory);
        if (defaultDir.exists() && defaultDir.isDirectory())
        {
            directoryList.setDirectory (defaultDir, true, false);
            loadLocalFolder (defaultDir);
            processorRef.triggerLibraryScan();
        }
    }

    startTimer (100);
    setSize (1000, 540);
}

ChopAudioProcessorEditor::~ChopAudioProcessorEditor()
{
    fileTree.removeListener (this);
    fileScannerThread.stopThread (2000);
}

void ChopAudioProcessorEditor::setStatus (const juce::String& text)
{
    statusLabel.setText (text, juce::dontSendNotification);
}

void ChopAudioProcessorEditor::doSearch()
{
    const auto query = searchBox.getText().trim();
    if (query.isEmpty())
        return;

    setStatus ("Searching local library for \"" + query + "\"…");

    auto allFiles = processorRef.getScannedFiles();
    juce::Array<Sample> matchedSamples;

    for (auto& f : allFiles)
    {
        if (f.getFileNameWithoutExtension().containsIgnoreCase (query))
        {
            Sample s;
            s.id = -1;
            s.name = f.getFileNameWithoutExtension();
            s.type = "LIBRARY";
            s.format = f.getFileExtension().removeCharacters (".");
            s.localFilePath = f.getFullPathName();
            s.durationMs = 0;
            matchedSamples.add (s);
        }
    }

    list->setSamples (matchedSamples);
    setStatus ("Found " + juce::String (matchedSamples.size()) + " sample(s)");
}


void ChopAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111114));
}

void ChopAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    // Status bar at the bottom spans the whole width
    statusLabel.setBounds (area.removeFromBottom (22));

    area.removeFromBottom (4); // minor spacer

    // Now divide the remaining space into left browser pane and right main pane
    auto leftPane = area.removeFromLeft (240);
    area.removeFromLeft (12); // spacer between panes
    auto rightPane = area;

    // Left pane layout
    auto buttonRow = leftPane.removeFromTop (28);
    selectFolderButton.setBounds (buttonRow.removeFromLeft (140));
    buttonRow.removeFromLeft (8);
    refreshButton.setBounds (buttonRow);
    leftPane.removeFromTop (8); // spacer
    fileTree.setBounds (leftPane);

    // Right pane layout
    // Top bar: title + search + stop buttons
    auto top = rightPane.removeFromTop (32);
    titleLabel.setBounds (top.removeFromLeft (70));
    stopButton.setBounds (top.removeFromRight (60).reduced (2, 0));
    searchButton.setBounds (top.removeFromRight (74).reduced (2, 0));
    searchBox.setBounds (top.reduced (2, 0));

    rightPane.removeFromTop (8); // spacer

    // List fills the rest of the right pane
    list->setBounds (rightPane);
}

void ChopAudioProcessorEditor::timerCallback()
{
    const bool isScanning = processorRef.isLibraryScanning();
    const int count = processorRef.getLibraryScannedCount();

    if (isScanning)
    {
        setStatus ("Indexing library... (" + juce::String (count) + " files found)");
        wasScanningLastCheck = true;
    }
    else if (wasScanningLastCheck)
    {
        setStatus ("Indexing complete. " + juce::String (count) + " samples ready.");
        wasScanningLastCheck = false;
    }
}

void ChopAudioProcessorEditor::fileClicked (const juce::File& file, const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    if (file.isDirectory())
    {
        loadLocalFolder (file);
    }
}

bool ChopAudioProcessorEditor::shouldDropFilesWhenDraggedExternally (
    const juce::DragAndDropTarget::SourceDetails& sourceDetails,
    juce::StringArray& files, bool& canMoveFiles)
{
    juce::ignoreUnused (sourceDetails, files, canMoveFiles);
    return false;
}

void ChopAudioProcessorEditor::selectLibraryFolder()
{
    chooser = std::make_unique<juce::FileChooser> (
        "Select your Sample Library Folder",
        juce::File::getSpecialLocation (juce::File::userHomeDirectory),
        juce::String(),
        true, // selectDirectory
        false // isForSave
    );

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectDirectories;

    juce::Component::SafePointer<ChopAudioProcessorEditor> safe (this);
    chooser->launchAsync (flags, [safe] (const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto result = fc.getResult();
        if (result.isDirectory())
        {
            safe->setLibraryFolder (result);
        }
    });
}

void ChopAudioProcessorEditor::setLibraryFolder (const juce::File& folder)
{
    processorRef.setLibraryFolder (folder);
    directoryList.setDirectory (folder, true, false);
    setStatus ("Library root set to: " + folder.getFileName());
}

void ChopAudioProcessorEditor::loadLocalFolder (const juce::File& folder)
{
    juce::Array<juce::File> foundFiles;
    folder.findChildFiles (foundFiles, juce::File::findFiles, false, "*.wav;*.mp3;*.aif;*.aiff;*.flac");

    juce::Array<Sample> localSamples;
    for (auto& f : foundFiles)
    {
        Sample s;
        s.id = -1;
        s.name = f.getFileNameWithoutExtension();
        s.type = "LIBRARY";
        s.format = f.getFileExtension().removeCharacters (".");
        s.localFilePath = f.getFullPathName();
        s.durationMs = 0;
        localSamples.add (s);
    }

    list->setSamples (localSamples);
    setStatus ("Folder \"" + folder.getFileName() + "\" · " + juce::String (localSamples.size()) + " sample(s)");
}
