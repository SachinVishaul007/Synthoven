#include "PluginEditor.h"

ChopAudioProcessorEditor::ChopAudioProcessorEditor (ChopAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
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
    importButton.onClick = [this] { doImport(); };
    stopButton.onClick   = [this] { processorRef.stopAudition(); setStatus ("Stopped"); };
    addAndMakeVisible (searchButton);
    addAndMakeVisible (importButton);
    addAndMakeVisible (stopButton);

    libraryButton.onClick   = [this] { loadSection ("/api/library/samples/library",   "Library"); };
    favoritesButton.onClick = [this] { loadSection ("/api/library/samples/favorites", "Favorites"); };
    generatedButton.onClick = [this] { loadSection ("/api/library/samples/generated", "Generated"); };
    recentButton.onClick    = [this] { loadSection ("/api/library/samples/recent",    "Recent"); };
    for (auto* b : { &libraryButton, &favoritesButton, &generatedButton, &recentButton })
        addAndMakeVisible (b);

    statusLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.6f));
    statusLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (statusLabel);

    list = std::make_unique<SampleListBox> (processorRef);
    list->onAudition = [this] (const Sample& s)
    {
        setStatus ("Loading \"" + s.name + "\"…");
        juce::Component::SafePointer<ChopAudioProcessorEditor> safe (this);
        processorRef.getApiClient().downloadSample (s.id, s.format,
            [safe, name = s.name] (juce::File f, juce::String error)
            {
                if (safe == nullptr) return;
                if (f.existsAsFile())
                {
                    safe->processorRef.auditionFile (f);
                    safe->setStatus ("Playing \"" + name + "\"");
                }
                else
                {
                    safe->setStatus (error.isNotEmpty() ? error : "Could not load sample");
                }
            });
    };
    list->onStatus = [this] (const juce::String& s) { setStatus (s); };
    addAndMakeVisible (*list);

    setSize (760, 540);

    // Show whatever is already in the library on open.
    loadSection ("/api/library/samples/library", "Library");
}

ChopAudioProcessorEditor::~ChopAudioProcessorEditor()
{
    stopTimer();
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

    setStatus ("Searching \"" + query + "\"…");
    pollingJobId = {};
    stopTimer();

    juce::Component::SafePointer<ChopAudioProcessorEditor> safe (this);
    processorRef.getApiClient().search (query,
        [safe] (ChopApiClient::SearchResult result, juce::String error)
        {
            if (safe == nullptr) return;
            if (error.isNotEmpty())
            {
                safe->setStatus ("Search failed — " + error);
                return;
            }
            safe->list->setSamples (result.libraryResults);
            safe->setStatus (juce::String (result.libraryResults.size()) + " library result(s)"
                             + (result.generationJobId.isNotEmpty() ? " · generating…" : ""));
            if (result.generationJobId.isNotEmpty())
                safe->startPolling (result.generationJobId);
        });
}

void ChopAudioProcessorEditor::startPolling (const juce::String& jobId)
{
    pollingJobId = jobId;
    pollAttempts = 0;
    startTimer (2000);
}

void ChopAudioProcessorEditor::timerCallback()
{
    if (pollingJobId.isEmpty())
    {
        stopTimer();
        return;
    }

    if (++pollAttempts > maxPollAttempts)
    {
        stopTimer();
        pollingJobId = {};
        setStatus ("Generation timed out");
        return;
    }

    juce::Component::SafePointer<ChopAudioProcessorEditor> safe (this);
    processorRef.getApiClient().getGenerationJob (pollingJobId,
        [safe] (ChopApiClient::JobResult job, juce::String error)
        {
            if (safe == nullptr) return;
            if (error.isNotEmpty())
            {
                safe->stopTimer();
                safe->pollingJobId = {};
                safe->setStatus ("Generation error — " + error);
                return;
            }

            if (job.status == "complete")
            {
                safe->stopTimer();
                safe->pollingJobId = {};
                safe->list->appendSamples (job.samples);
                safe->setStatus ("Generated " + juce::String (job.samples.size()) + " sample(s)");
            }
            else if (job.status == "failed")
            {
                safe->stopTimer();
                safe->pollingJobId = {};
                safe->setStatus ("Generation failed" + (job.error.isNotEmpty() ? (" — " + job.error) : juce::String()));
            }
            // queued / generating → keep polling
        });
}

void ChopAudioProcessorEditor::loadSection (const juce::String& path, const juce::String& label)
{
    setStatus ("Loading " + label + "…");
    pollingJobId = {};
    stopTimer();

    juce::Component::SafePointer<ChopAudioProcessorEditor> safe (this);
    processorRef.getApiClient().getSamples (path,
        [safe, label] (juce::Array<Sample> samples, juce::String error)
        {
            if (safe == nullptr) return;
            if (error.isNotEmpty())
            {
                safe->setStatus (label + " failed — " + error);
                return;
            }
            safe->list->setSamples (samples);
            safe->setStatus (label + " · " + juce::String (samples.size()) + " sample(s)");
        });
}

void ChopAudioProcessorEditor::doImport()
{
    chooser = std::make_unique<juce::FileChooser> (
        "Select audio files to import",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory),
        "*.wav;*.mp3;*.aiff;*.aif;*.flac");

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::canSelectMultipleItems;

    juce::Component::SafePointer<ChopAudioProcessorEditor> safe (this);
    chooser->launchAsync (flags, [safe] (const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto results = fc.getResults();
        if (results.isEmpty())
            return;

        juce::Array<juce::File> files;
        for (auto& f : results)
            files.add (f);

        safe->setStatus ("Uploading " + juce::String (files.size()) + " file(s)…");
        safe->processorRef.getApiClient().uploadFiles (files,
            [safe] (juce::Array<Sample> imported, juce::String error)
            {
                if (safe == nullptr) return;
                if (error.isNotEmpty())
                {
                    safe->setStatus ("Upload failed — " + error);
                    return;
                }
                safe->list->setSamples (imported);
                safe->setStatus ("Imported " + juce::String (imported.size()) + " sample(s)");
            });
    });
}

void ChopAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111114));
}

void ChopAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    // Top bar: title + search + buttons
    auto top = area.removeFromTop (32);
    titleLabel.setBounds (top.removeFromLeft (70));
    importButton.setBounds (top.removeFromRight (70));
    searchButton.setBounds (top.removeFromRight (74).reduced (2, 0));
    searchBox.setBounds (top.reduced (2, 0));

    area.removeFromTop (8);

    // Section row
    auto sectionRow = area.removeFromTop (28);
    auto sectionW = sectionRow.getWidth() / 5;
    libraryButton.setBounds   (sectionRow.removeFromLeft (sectionW).reduced (2, 0));
    favoritesButton.setBounds (sectionRow.removeFromLeft (sectionW).reduced (2, 0));
    generatedButton.setBounds (sectionRow.removeFromLeft (sectionW).reduced (2, 0));
    recentButton.setBounds    (sectionRow.removeFromLeft (sectionW).reduced (2, 0));
    stopButton.setBounds      (sectionRow.reduced (2, 0));

    area.removeFromTop (8);

    // Status bar at the bottom
    statusLabel.setBounds (area.removeFromBottom (22));

    // List fills the rest
    list->setBounds (area);
}
