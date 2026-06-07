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

    semanticSearchToggle.setButtonText ("AI Search");
    semanticSearchToggle.setToggleState (processorRef.isSemanticSearchEnabled(), juce::dontSendNotification);
    semanticSearchToggle.onClick = [this]
    {
        processorRef.setSemanticSearchEnabled (semanticSearchToggle.getToggleState());
        doSearch();
    };
    semanticSearchToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::white.withAlpha (0.9f));
    semanticSearchToggle.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffe0a458));
    addAndMakeVisible (semanticSearchToggle);

    for (const auto& tag : tags)
    {
        auto btn = std::make_unique<juce::TextButton> (tag);
        btn->setButtonText (tag);
        btn->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202024));
        btn->setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.8f));
        btn->onClick = [this, tag]
        {
            searchBox.setText (tag, juce::dontSendNotification);
            doSearch();
            updateTagHighlighting (tag);
        };
        addAndMakeVisible (*btn);
        tagButtons.push_back (std::move (btn));
    }

    // ── Generation setup panel ─────────────────────────────────────────────
    auto sectionHeader = [] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
        l.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.45f));
    };

    genHeaderLabel.setText ("GENERATION SETUP", juce::dontSendNotification);
    genHeaderLabel.setFont (juce::Font (juce::FontOptions (13.0f).withStyle ("Bold")));
    genHeaderLabel.setColour (juce::Label::textColourId, juce::Colour (0xff19c3b3));
    addAndMakeVisible (genHeaderLabel);

    sectionHeader (promptHeaderLabel, "TEXT PROMPT");
    addAndMakeVisible (promptHeaderLabel);

    promptEditor.setMultiLine (true, true);
    promptEditor.setReturnKeyStartsNewLine (true);
    promptEditor.setTextToShowWhenEmpty ("Describe the sound…", juce::Colours::white.withAlpha (0.35f));
    promptEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff202024));
    promptEditor.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff34343c));
    addAndMakeVisible (promptEditor);

    sectionHeader (durationHeaderLabel, "MAX DURATION");
    addAndMakeVisible (durationHeaderLabel);

    durationSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    durationSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    durationSlider.setRange (1.0, 30.0, 0.1);
    durationSlider.setValue (8.0, juce::dontSendNotification);
    durationSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff19c3b3));
    durationSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff19c3b3));
    durationSlider.onValueChange = [this]
    {
        durationValueLabel.setText (juce::String (durationSlider.getValue(), 1) + "s",
                                    juce::dontSendNotification);
    };
    addAndMakeVisible (durationSlider);

    durationValueLabel.setText ("8.0s", juce::dontSendNotification);
    durationValueLabel.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Bold")));
    durationValueLabel.setColour (juce::Label::textColourId, juce::Colour (0xff19c3b3));
    durationValueLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (durationValueLabel);

    generateSoundButton.onClick = [this] { doGenerate(); };
    generateSoundButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff19c3b3));
    generateSoundButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff0b1f1d));
    addAndMakeVisible (generateSoundButton);

    // Audio visualizer — a live waveform of whatever is being auditioned.
    sectionHeader (visualizerHeaderLabel, "NOW PLAYING");
    addAndMakeVisible (visualizerHeaderLabel);
    addAndMakeVisible (processorRef.getVisualiser());

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
    setSize (1180, 560);
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

    updateTagHighlighting (query);

    setStatus ("Searching local library for \"" + query + "\"…");

    auto matchedFiles = processorRef.performSearch (query);
    juce::Array<Sample> matchedSamples;

    for (auto& f : matchedFiles)
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

    list->setSamples (matchedSamples);
    setStatus ("Found " + juce::String (matchedSamples.size()) + " sample(s)");
}

juce::File ChopAudioProcessorEditor::getGeneratedFolder() const
{
    // Mirrors the backend default chop.generation.output-path (~/Music/Chop/Generated).
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory)
               .getChildFile ("Chop")
               .getChildFile ("Generated");
}

void ChopAudioProcessorEditor::doGenerate()
{
    const auto prompt = promptEditor.getText().trim();
    if (prompt.isEmpty())
    {
        setStatus ("Describe the sound first, e.g. \"warm analog bass\"");
        return;
    }

    const double duration = durationSlider.getValue();
    // Fixed CFG scale tuned for good prompt adherence without over-constraining.
    const double cfgScale = 3.0;
    const juce::String category; // category control removed; rely on the prompt alone

    setStatus ("Generating \"" + prompt + "\" (" + juce::String (duration, 1)
               + "s)… this can take a little while");
    generateSoundButton.setEnabled (false);

    juce::Component::SafePointer<ChopAudioProcessorEditor> safe (this);
    processorRef.getApiClient().generate (prompt, duration, cfgScale, category,
        [safe, prompt] (ChopApiClient::JobResult result, juce::String error)
    {
        if (safe == nullptr)
            return;

        safe->generateSoundButton.setEnabled (true);

        if (error.isNotEmpty())
        {
            safe->setStatus ("Generation failed: " + error);
            return;
        }

        // The backend saved the new audio into the Generated folder; show that
        // folder so the fresh files appear and can be auditioned / dragged to a DAW.
        auto folder = safe->getGeneratedFolder();
        if (folder.isDirectory())
        {
            safe->directoryList.setDirectory (folder, true, false);
            safe->loadLocalFolder (folder);
            safe->processorRef.triggerLibraryScan();
        }

        safe->setStatus ("Generated " + juce::String (result.samples.size())
                         + " sample(s) for \"" + prompt + "\" → " + folder.getFullPathName());
    });
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

    // Three columns: folder browser (left) | results (centre) | generation (right)
    auto leftPane = area.removeFromLeft (240);
    area.removeFromLeft (12); // spacer
    auto genPanel = area.removeFromRight (270);
    area.removeFromRight (12); // spacer
    auto centrePane = area;

    // Left pane layout
    auto buttonRow = leftPane.removeFromTop (28);
    selectFolderButton.setBounds (buttonRow.removeFromLeft (140));
    buttonRow.removeFromLeft (8);
    refreshButton.setBounds (buttonRow);
    leftPane.removeFromTop (8); // spacer
    fileTree.setBounds (leftPane);

    // Centre pane: top bar (title + search + AI toggle + stop), tag row, results list
    auto top = centrePane.removeFromTop (32);
    titleLabel.setBounds (top.removeFromLeft (70));
    stopButton.setBounds (top.removeFromRight (56).reduced (2, 0));
    searchButton.setBounds (top.removeFromRight (66).reduced (2, 0));
    semanticSearchToggle.setBounds (top.removeFromRight (85).reduced (2, 0));
    searchBox.setBounds (top.reduced (2, 0));
    centrePane.removeFromTop (8); // spacer

    // Tag buttons row
    auto tagRow = centrePane.removeFromTop (28);
    for (auto& btn : tagButtons)
    {
        btn->setBounds (tagRow.removeFromLeft (75));
        tagRow.removeFromLeft (6);
    }
    centrePane.removeFromTop (8); // spacer

    // Audio visualizer strip pinned to the bottom of the centre pane.
    {
        auto vizArea = centrePane.removeFromBottom (90);
        visualizerHeaderLabel.setBounds (vizArea.removeFromTop (16));
        processorRef.getVisualiser().setBounds (vizArea);
        centrePane.removeFromBottom (8); // spacer above the visualizer
    }

    list->setBounds (centrePane);

    // Right pane: generation setup panel
    genHeaderLabel.setBounds (genPanel.removeFromTop (22));
    genPanel.removeFromTop (8);

    promptHeaderLabel.setBounds (genPanel.removeFromTop (16));
    promptEditor.setBounds (genPanel.removeFromTop (110));
    genPanel.removeFromTop (12);

    durationHeaderLabel.setBounds (genPanel.removeFromTop (16));
    {
        auto durRow = genPanel.removeFromTop (24);
        durationValueLabel.setBounds (durRow.removeFromRight (52));
        durationSlider.setBounds (durRow);
    }
    genPanel.removeFromTop (16);

    generateSoundButton.setBounds (genPanel.removeFromTop (40));
}

void ChopAudioProcessorEditor::timerCallback()
{
    auto& modelMgr = processorRef.getModelManager();
    
    if (modelMgr.isDownloading())
    {
        setStatus (modelMgr.getStatusMessage());
        wasScanningLastCheck = true;
    }
    else if (processorRef.isLibraryScanning())
    {
        const int count = processorRef.getLibraryScannedCount();
        setStatus ("Indexing library... (" + juce::String (count) + " files found)");
        wasScanningLastCheck = true;
    }
    else if (processorRef.isEmbeddingIndexing())
    {
        const int current = processorRef.getEmbeddingIndexedCount();
        const int total = processorRef.getEmbeddingTotalCount();
        setStatus ("Indexing AI features: " + juce::String (current) + " / " + juce::String (total));
        wasScanningLastCheck = true;
    }
    else if (wasScanningLastCheck)
    {
        const int count = processorRef.getLibraryScannedCount();
        if (modelMgr.isModelLoaded())
            setStatus ("Library ready. " + juce::String (count) + " samples indexed with AI Search.");
        else
            setStatus ("Library ready. " + juce::String (count) + " samples indexed (AI Search disabled).");
        wasScanningLastCheck = false;
    }
}

void ChopAudioProcessorEditor::updateTagHighlighting (const juce::String& activeTag)
{
    for (auto& btn : tagButtons)
    {
        if (btn->getButtonText().equalsIgnoreCase (activeTag))
        {
            btn->setColour (juce::TextButton::buttonColourId, juce::Colour (0xffe0a458));
            btn->setColour (juce::TextButton::textColourOffId, juce::Colours::black);
        }
        else
        {
            btn->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202024));
            btn->setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.8f));
        }
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
