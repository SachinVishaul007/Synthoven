#include "PluginEditor.h"

ChopAudioProcessorEditor::ChopAudioProcessorEditor (ChopAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      folderFilter(),
      fileScannerThread ("ChopFileScanner"),
      directoryList (&folderFilter, fileScannerThread),
      fileTree (directoryList),
      waveformPreview (p)
{
    titleLabel.setText ("CHOP", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (20.0f).withStyle ("Bold")));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe0a458));
    titleLabel.setTitle ("Chop Sample Browser Title");
    titleLabel.setDescription ("Application Header");
    addAndMakeVisible (titleLabel);

    searchBox.setTextToShowWhenEmpty ("Search samples, e.g. punchy techno kick",
                                      juce::Colours::white.withAlpha (0.4f));
    searchBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff202024));
    searchBox.onReturnKey = [this] { doSearch(); };
    searchBox.setTitle ("Search Text Field");
    searchBox.setDescription ("Enter search keywords or semantic descriptions to search the library");
    searchBox.setHelpText ("Press Enter to execute the search");
    addAndMakeVisible (searchBox);

    searchButton.onClick = [this] { doSearch(); };
    searchButton.setTitle ("Search Button");
    searchButton.setDescription ("Executes text or semantic search depending on toggle state");

    addAndMakeVisible (searchButton);

    semanticSearchToggle.setButtonText ("AI Search");
    semanticSearchToggle.setToggleState (processorRef.isSemanticSearchEnabled(), juce::dontSendNotification);
    semanticSearchToggle.onClick = [this]
    {
        processorRef.setSemanticSearchEnabled (semanticSearchToggle.getToggleState());
        doSearch();
    };
    semanticSearchToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::white.withAlpha (0.9f));
    semanticSearchToggle.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffe0a458));
    semanticSearchToggle.setTitle ("AI Search Toggle");
    semanticSearchToggle.setDescription ("Enable to run an AI-based semantic search instead of classic keyword matching");
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
        btn->setTitle (tag + " Tag Button");
        btn->setDescription ("Filters search results to only show samples tagged with " + tag);
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
    genHeaderLabel.setTitle ("Generation Setup Header");
    genHeaderLabel.setDescription ("Provides controls to configure and generate AI samples using Stable Audio");
    addAndMakeVisible (genHeaderLabel);

    sectionHeader (promptHeaderLabel, "TEXT PROMPT");
    addAndMakeVisible (promptHeaderLabel);

    promptEditor.setMultiLine (true, true);
    promptEditor.setReturnKeyStartsNewLine (true);
    promptEditor.setTextToShowWhenEmpty ("Describe the sound…", juce::Colours::white.withAlpha (0.35f));
    promptEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff202024));
    promptEditor.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff34343c));
    promptEditor.setTitle ("Text Prompt Editor");
    promptEditor.setDescription ("Type text description of the sound to generate");
    promptEditor.setHelpText ("e.g. warm analog bass");
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
    durationSlider.setTitle ("Max Duration Slider");
    durationSlider.setDescription ("Adjusts the maximum duration of the generated sample in seconds");
    addAndMakeVisible (durationSlider);

    durationValueLabel.setText ("8.0s", juce::dontSendNotification);
    durationValueLabel.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Bold")));
    durationValueLabel.setColour (juce::Label::textColourId, juce::Colour (0xff19c3b3));
    durationValueLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (durationValueLabel);

    sectionHeader (audioInputHeaderLabel, "SOURCE AUDIO (FOR AUDIO-TO-AUDIO)");
    audioInputHeaderLabel.setTitle ("Source Audio Label");
    audioInputHeaderLabel.setDescription ("Label for the audio-to-audio drag and drop input field");
    addAndMakeVisible (audioInputHeaderLabel);

    audioInputDropTarget.setTitle ("Audio Input Drop Zone");
    audioInputDropTarget.setDescription ("Drag and drop a WAV, MP3, AIFF, or FLAC file here to use it as the source for audio-to-audio generation");
    addAndMakeVisible (audioInputDropTarget);

    generateSoundButton.onClick = [this] { doGenerate(); };
    generateSoundButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff19c3b3));
    generateSoundButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff0b1f1d));
    generateSoundButton.setTitle ("Generate Sound Button");
    generateSoundButton.setDescription ("Triggers AI stable audio sample generation with the specified prompt and duration");
    addAndMakeVisible (generateSoundButton);

    // Audio visualizer — a live waveform of whatever is being auditioned.
    sectionHeader (visualizerHeaderLabel, "NOW PLAYING");
    visualizerHeaderLabel.setTitle ("Now Playing Waveform Visualizer");
    visualizerHeaderLabel.setDescription ("Displays the real-time waveform of the currently playing or auditioned audio sample");
    addAndMakeVisible (visualizerHeaderLabel);
    addAndMakeVisible (waveformPreview);

    statusLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.6f));
    statusLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    statusLabel.setTitle ("Status Label");
    statusLabel.setDescription ("Displays status information such as generation or search progress");
    addAndMakeVisible (statusLabel);

    list = std::make_unique<SampleListBox> (processorRef);
    auto setupList = [this](std::unique_ptr<SampleListBox>& lst) {
        if (!lst) lst = std::make_unique<SampleListBox> (processorRef);
        lst->onAudition = [this] (const Sample& s) {
            if (s.localFilePath.isNotEmpty()) {
                juce::File f (s.localFilePath);
                if (f.existsAsFile()) {
                    processorRef.auditionFile (f);
                    waveformPreview.setSampleFile (f);
                    setStatus ("Playing local file: " + s.name);
                }
            }
        };
        lst->onStatus = [this] (const juce::String& s) { setStatus (s); };
        lst->onFavoriteToggled = [this] (const Sample& s) {
            if (currentTab == TabMode::Favorites) loadFavoriteSamples();
            list->repaint();
            if (generatedList) generatedList->repaint();
        };
        addChildComponent (*lst);
    };

    setupList (list);
    list->setVisible (true); // Center list is always visible
    setupList (generatedList);
    setupList (favoritesList);

    // Sidebar Tab Buttons
    auto setupTabButton = [this](juce::TextButton& btn, TabMode mode) {
        btn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1c1c20));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe0a458));
        btn.setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.7f));
        btn.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        btn.onClick = [this, mode] { setTabMode (mode); };
        addAndMakeVisible (btn);
    };
    setupTabButton (tabMySounds, TabMode::MySounds);
    setupTabButton (tabGenerated, TabMode::Generated);
    setupTabButton (tabFavorites, TabMode::Favorites);

    settingsButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202024));
    settingsButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.8f));
    settingsButton.onClick = [this] { showSettingsPopup(); };
    addAndMakeVisible (settingsButton);

    // Setup local library browser components
    selectFolderButton.onClick = [this] { selectLibraryFolder(); };
    selectFolderButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202024));
    selectFolderButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    selectFolderButton.setTitle ("Select Folder Button");
    selectFolderButton.setDescription ("Opens a folder chooser to select a local directory containing audio files");
    addAndMakeVisible (selectFolderButton);

    refreshButton.onClick = [this] {
        processorRef.triggerLibraryScan();
        directoryList.refresh();
    };
    refreshButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202024));
    refreshButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    refreshButton.setTitle ("Refresh Button");
    refreshButton.setDescription ("Triggers a manual scan of the current directory to refresh the list of local files");
    addAndMakeVisible (refreshButton);

    fileTree.setColour (juce::TreeView::backgroundColourId, juce::Colour (0xff161619));
    fileTree.setColour (juce::TreeView::linesColourId, juce::Colours::white.withAlpha (0.1f));
    fileTree.setColour (juce::TreeView::selectedItemBackgroundColourId, juce::Colour (0xffe0a458).withAlpha (0.2f));
    fileTree.setColour (juce::DirectoryContentsDisplayComponent::textColourId, juce::Colours::white.withAlpha (0.9f));
    fileTree.setColour (juce::DirectoryContentsDisplayComponent::highlightedTextColourId, juce::Colours::white);

    fileTree.addListener (this);
    fileTree.setDragAndDropDescription ("ChopLocalFile");
    fileTree.setTitle ("Local library folder tree");
    fileTree.setDescription ("Displays directory structure of the loaded local samples folder, allowing selection of folders to display");
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

    setTabMode (TabMode::MySounds);

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

    setTabMode (TabMode::MySounds);

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
    juce::String searchResult = "Found " + juce::String (matchedSamples.size()) + " sample(s)";
    setStatus (searchResult);
    juce::AccessibilityHandler::postAnnouncement ("Search completed. " + searchResult, juce::AccessibilityHandler::AnnouncementPriority::high);
}

void ChopAudioProcessorEditor::setTabMode (TabMode newMode)
{
    currentTab = newMode;
    
    tabMySounds.setToggleState (newMode == TabMode::MySounds, juce::dontSendNotification);
    tabGenerated.setToggleState (newMode == TabMode::Generated, juce::dontSendNotification);
    tabFavorites.setToggleState (newMode == TabMode::Favorites, juce::dontSendNotification);

    fileTree.setVisible (newMode == TabMode::MySounds);
    selectFolderButton.setVisible (newMode == TabMode::MySounds);
    refreshButton.setVisible (newMode == TabMode::MySounds);

    if (generatedList) generatedList->setVisible (newMode == TabMode::Generated);
    if (favoritesList) favoritesList->setVisible (newMode == TabMode::Favorites);

    if (newMode == TabMode::Generated)
        loadGeneratedSamples();
    else if (newMode == TabMode::Favorites)
        loadFavoriteSamples();
        
    resized();
}

void ChopAudioProcessorEditor::loadGeneratedSamples()
{
    auto folder = processorRef.getGeneratedFolder();
    juce::Array<juce::File> foundFiles;
    folder.findChildFiles (foundFiles, juce::File::findFiles, false, "*.wav;*.mp3;*.aif;*.aiff;*.flac");

    std::sort (foundFiles.begin(), foundFiles.end(), [] (const juce::File& a, const juce::File& b) {
        return a.getLastModificationTime() > b.getLastModificationTime();
    });

    juce::Array<Sample> samples;
    for (auto& f : foundFiles)
    {
        Sample s;
        s.id = -1;
        s.name = f.getFileNameWithoutExtension();
        s.type = "GENERATED";
        s.format = f.getFileExtension().removeCharacters (".");
        s.localFilePath = f.getFullPathName();
        s.durationMs = 0;
        samples.add (s);
    }
    if (generatedList) generatedList->setSamples (samples);
}

void ChopAudioProcessorEditor::loadFavoriteSamples()
{
    juce::StringArray favs = processorRef.getFavoritePaths();
    juce::Array<Sample> samples;
    for (const auto& path : favs)
    {
        juce::File f (path);
        if (f.existsAsFile())
        {
            Sample s;
            s.id = -1;
            s.name = f.getFileNameWithoutExtension();
            s.type = "FAVORITE";
            s.format = f.getFileExtension().removeCharacters (".");
            s.localFilePath = f.getFullPathName();
            s.durationMs = 0;
            samples.add (s);
        }
    }
    if (favoritesList) favoritesList->setSamples (samples);
}

class SettingsPopupComponent : public juce::Component
{
public:
    SettingsPopupComponent (ChopAudioProcessor& p, ChopAudioProcessorEditor& editor)
        : processor (p), editorRef (editor)
    {
        setSize (340, 240);

        headerLabel.setText ("PLUGIN CONFIGURATION", juce::dontSendNotification);
        headerLabel.setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold")));
        headerLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe0a458));
        addAndMakeVisible (headerLabel);

        apiLabel.setText ("Backend API URL:", juce::dontSendNotification);
        apiLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
        addAndMakeVisible (apiLabel);

        apiEditor.setText (processor.getApiClient().getBaseUrl(), juce::dontSendNotification);
        apiEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff202024));
        apiEditor.setColour (juce::TextEditor::outlineColourId, juce::Colours::white.withAlpha (0.15f));
        apiEditor.onTextChange = [this] {
            processor.getApiClient().setBaseUrl (apiEditor.getText().trim());
        };
        addAndMakeVisible (apiEditor);

        libLabel.setText ("Library Folder:", juce::dontSendNotification);
        libLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
        addAndMakeVisible (libLabel);

        updateLibraryText();
        libPathLabel.setColour (juce::Label::backgroundColourId, juce::Colour (0xff202024));
        libPathLabel.setColour (juce::Label::outlineColourId, juce::Colours::white.withAlpha (0.15f));
        libPathLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
        addAndMakeVisible (libPathLabel);

        changeLibButton.setButtonText ("Change...");
        changeLibButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2a30));
        changeLibButton.onClick = [this] {
            editorRef.selectLibraryFolder();
            updateLibraryText();
        };
        addAndMakeVisible (changeLibButton);

        statsLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
        statsLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.6f));
        addAndMakeVisible (statsLabel);

        updateStats();

        closeButton.setButtonText ("Done");
        closeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffe0a458));
        closeButton.setColour (juce::TextButton::textColourOffId, juce::Colours::black);
        closeButton.onClick = [this] {
            if (auto* callout = findParentComponentOfClass<juce::CallOutBox>())
                callout->dismiss();
        };
        addAndMakeVisible (closeButton);
    }

    void updateLibraryText()
    {
        auto path = processor.getLibraryFolder().getFullPathName();
        if (path.isEmpty())
            path = "None selected";
        libPathLabel.setText (path, juce::dontSendNotification);
    }

    void updateStats()
    {
        juce::String text = "Scanned: " + juce::String (processor.getScannedFiles().size()) + " files\n";
        text << "AI Indexing: " << juce::String (processor.getEmbeddingIndexedCount())
             << " / " << juce::String (processor.getEmbeddingTotalCount());
        statsLabel.setText (text, juce::dontSendNotification);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12);
        headerLabel.setBounds (area.removeFromTop (24));
        area.removeFromTop (12);

        auto apiArea = area.removeFromTop (44);
        apiLabel.setBounds (apiArea.removeFromTop (16));
        apiEditor.setBounds (apiArea.removeFromTop (24));
        area.removeFromTop (12);

        auto libArea = area.removeFromTop (48);
        libLabel.setBounds (libArea.removeFromTop (16));
        auto libRow = libArea.removeFromTop (28);
        changeLibButton.setBounds (libRow.removeFromRight (70));
        libRow.removeFromRight (6);
        libPathLabel.setBounds (libRow);
        area.removeFromTop (12);

        auto bottomRow = area;
        closeButton.setBounds (bottomRow.removeFromRight (60));
        statsLabel.setBounds (bottomRow);
    }

private:
    ChopAudioProcessor& processor;
    ChopAudioProcessorEditor& editorRef;

    juce::Label headerLabel;
    juce::Label apiLabel;
    juce::TextEditor apiEditor;

    juce::Label libLabel;
    juce::Label libPathLabel;
    juce::TextButton changeLibButton;

    juce::Label statsLabel;
    juce::TextButton closeButton;
};

void ChopAudioProcessorEditor::showSettingsPopup()
{
    auto* settingsComp = new SettingsPopupComponent (processorRef, *this);
    juce::CallOutBox::launchAsynchronously (std::unique_ptr<juce::Component> (settingsComp), 
                                            settingsButton.getScreenBounds(), 
                                            nullptr);
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

    const auto initAudio = audioInputDropTarget.getInputFile();
    if (initAudio.existsAsFile())
    {
        setStatus ("Generating similar sound (" + juce::String (duration, 1)
                   + "s) using audio input…");
        generateSoundButton.setEnabled (false);

        juce::Component::SafePointer<ChopAudioProcessorEditor> safe (this);
        processorRef.getApiClient().generateAudioToAudio (prompt, duration, cfgScale, category, initAudio,
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

            auto folder = safe->processorRef.getGeneratedFolder();
            safe->processorRef.triggerLibraryScan();
            safe->loadGeneratedSamples();

            safe->setStatus ("Generated similar sample(s) for \"" + prompt + "\"");
        });
    }
    else
    {
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

            auto folder = safe->processorRef.getGeneratedFolder();
            safe->processorRef.triggerLibraryScan();
            safe->loadGeneratedSamples();

            safe->setStatus ("Generated " + juce::String (result.samples.size())
                             + " sample(s) for \"" + prompt + "\"");
        });
    }
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

    // Left pane layout: accordion style
    settingsButton.setBounds (leftPane.removeFromBottom (28));
    leftPane.removeFromBottom (8); // spacer

    if (currentTab == TabMode::MySounds)
    {
        tabMySounds.setBounds (leftPane.removeFromTop (32));
        leftPane.removeFromTop (6);

        auto folderArea = leftPane;
        // Leave space for tabGenerated (32px + 6px spacer) and tabFavorites (32px)
        folderArea.removeFromBottom (32 + 6 + 32);

        auto buttonRow = folderArea.removeFromTop (28);
        selectFolderButton.setBounds (buttonRow.removeFromLeft (140));
        buttonRow.removeFromLeft (8);
        refreshButton.setBounds (buttonRow);
        folderArea.removeFromTop (8); // spacer
        fileTree.setBounds (folderArea);

        // Position the collapsed tabs at the bottom
        tabGenerated.setBounds (leftPane.removeFromBottom (32 + 6 + 32).removeFromTop (32));
        tabFavorites.setBounds (leftPane.removeFromBottom (32));
    }
    else
    {
        tabMySounds.setBounds (leftPane.removeFromTop (32));
        leftPane.removeFromTop (6);
        tabGenerated.setBounds (leftPane.removeFromTop (32));
        leftPane.removeFromTop (6);
        tabFavorites.setBounds (leftPane.removeFromTop (32));
    }

    // Centre pane: top bar (title + search + AI toggle), tag row, results list
    auto top = centrePane.removeFromTop (32);
    titleLabel.setBounds (top.removeFromLeft (70));
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

    // Waveform preview component pinned to the bottom of the centre pane.
    {
        auto vizArea = centrePane.removeFromBottom (90);
        visualizerHeaderLabel.setBounds (vizArea.removeFromTop (16));
        waveformPreview.setBounds (vizArea);
        centrePane.removeFromBottom (8); // spacer above the visualizer
    }

    // Map list, generatedList, and favoritesList to centrePane bounds (only active one will be visible)
    list->setBounds (centrePane);
    if (generatedList) generatedList->setBounds (centrePane);
    if (favoritesList) favoritesList->setBounds (centrePane);

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
    genPanel.removeFromTop (12);

    audioInputHeaderLabel.setBounds (genPanel.removeFromTop (16));
    audioInputDropTarget.setBounds (genPanel.removeFromTop (36));
    genPanel.removeFromTop (12);

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
        juce::String readyMsg = "Library ready. " + juce::String (count) + " samples indexed.";
        if (modelMgr.isModelLoaded())
            readyMsg += " AI Search enabled.";
        else
            readyMsg += " AI Search disabled.";

        setStatus (readyMsg);
        juce::AccessibilityHandler::postAnnouncement (readyMsg, juce::AccessibilityHandler::AnnouncementPriority::high);
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
