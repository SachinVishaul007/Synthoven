#include "PluginEditor.h"
#include <optional>
#include <vector>

ChopAudioProcessorEditor::ChopAudioProcessorEditor (ChopAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    // Define the offline resource provider to load files from FRONTEND_DIST_DIR
    auto getResourceForUrl = [] (const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
    {
        juce::String path = url;
        if (path.startsWith ("http://localhost"))
            path = path.substring (16); // Remove "http://localhost"
        
        if (path.isEmpty() || path == "/")
            path = "/index.html";

        juce::File rootDir (FRONTEND_DIST_DIR);
        juce::File file = rootDir.getChildFile (path);
        
        if (file.existsAsFile())
        {
            juce::MemoryBlock mb;
            juce::FileInputStream stream (file);
            if (stream.openedOk())
            {
                mb.setSize (file.getSize());
                stream.read (mb.getData(), mb.getSize());
                juce::String mimeType = "text/plain";
                auto ext = file.getFileExtension().toLowerCase();
                if (ext == ".html" || ext == ".htm") mimeType = "text/html";
                else if (ext == ".js")               mimeType = "application/javascript";
                else if (ext == ".css")              mimeType = "text/css";
                else if (ext == ".png")              mimeType = "image/png";
                else if (ext == ".jpg" || ext == ".jpeg") mimeType = "image/jpeg";
                else if (ext == ".svg")              mimeType = "image/svg+xml";
                else if (ext == ".wav")              mimeType = "audio/wav";
                else if (ext == ".mp3")              mimeType = "audio/mpeg";

                std::vector<std::byte> byteVec (mb.getSize());
                std::memcpy (byteVec.data(), mb.getData(), mb.getSize());
                return juce::WebBrowserComponent::Resource { std::move (byteVec), mimeType };
            }
        }
        return std::nullopt;
    };

    auto options = juce::WebBrowserComponent::Options()
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2()
            .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)))
        .withNativeIntegrationEnabled (true)
        .withResourceProvider (getResourceForUrl);

    // ── 1. Search Function ──
    options = options.withNativeFunction ("search", [this] (const juce::Array<juce::var>& args, auto completion)
    {
        if (args.size() > 0)
        {
            juce::String query = args[0].toString();
            auto matchedFiles = processorRef.performSearch (query);
            
            juce::Array<juce::var> matchedSamples;
            for (auto& f : matchedFiles)
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty ("id", f.hashCode());
                obj->setProperty ("name", f.getFileNameWithoutExtension());
                obj->setProperty ("type", "library");
                obj->setProperty ("format", f.getFileExtension().removeCharacters ("."));
                obj->setProperty ("filePath", f.getFullPathName());
                obj->setProperty ("durationMs", 0);
                matchedSamples.add (juce::var (obj));
            }
            completion (matchedSamples);
            return;
        }
        completion (juce::Array<juce::var>());
    });

    // ── 2. Get Library Samples ──
    options = options.withNativeFunction ("getLibrarySamples", [this] (const juce::Array<juce::var>& args, auto completion)
    {
        juce::ignoreUnused (args);
        auto savedFolder = processorRef.getLibraryFolder();
        if (!savedFolder.exists() || !savedFolder.isDirectory())
        {
            savedFolder = juce::File::getSpecialLocation (juce::File::userMusicDirectory);
        }
        
        juce::Array<juce::File> foundFiles;
        savedFolder.findChildFiles (foundFiles, juce::File::findFiles, false, "*.wav;*.mp3;*.aif;*.aiff;*.flac");
        
        juce::Array<juce::var> localSamples;
        for (auto& f : foundFiles)
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("id", f.hashCode());
            obj->setProperty ("name", f.getFileNameWithoutExtension());
            obj->setProperty ("type", "library");
            obj->setProperty ("format", f.getFileExtension().removeCharacters ("."));
            obj->setProperty ("filePath", f.getFullPathName());
            obj->setProperty ("durationMs", 0);
            localSamples.add (juce::var (obj));
        }
        completion (localSamples);
    });

    // ── 3. Play Sample ──
    options = options.withNativeFunction ("playSample", [this] (const juce::Array<juce::var>& args, auto completion)
    {
        if (args.size() > 0)
        {
            juce::String path = args[0].toString();
            juce::File f (path);
            if (f.existsAsFile())
            {
                processorRef.auditionFile (f);
            }
        }
        completion (juce::var());
    });

    // ── 4. Stop Audition ──
    options = options.withNativeFunction ("stopAudition", [this] (const juce::Array<juce::var>& args, auto completion)
    {
        juce::ignoreUnused (args);
        processorRef.stopAudition();
        completion (juce::var());
    });

    // ── 5. Select Library Folder ──
    options = options.withNativeFunction ("selectLibraryFolder", [this] (const juce::Array<juce::var>& args, auto completion)
    {
        juce::ignoreUnused (args);
        juce::MessageManager::callAsync ([this]
        {
            selectLibraryFolder();
        });
        completion (juce::var());
    });

    // ── 6. Get Library/Indexing Status ──
    options = options.withNativeFunction ("getLibraryStatus", [this] (const juce::Array<juce::var>& args, auto completion)
    {
        juce::ignoreUnused (args);
        auto* obj = new juce::DynamicObject();
        
        bool isScanning = processorRef.isLibraryScanning() || processorRef.isEmbeddingIndexing();
        obj->setProperty ("isScanning", isScanning);
        obj->setProperty ("fileCount", processorRef.getLibraryScannedCount());
        obj->setProperty ("embeddingIndexedCount", processorRef.getEmbeddingIndexedCount());
        obj->setProperty ("embeddingTotalCount", processorRef.getEmbeddingTotalCount());
        
        auto& modelMgr = processorRef.getModelManager();
        obj->setProperty ("isModelLoaded", modelMgr.isModelLoaded());
        obj->setProperty ("modelStatusMessage", modelMgr.getStatusMessage());
        
        completion (juce::var (obj));
    });

    // ── 7. Drag Sample ──
    options = options.withNativeFunction ("dragSample", [this] (const juce::Array<juce::var>& args, auto completion)
    {
        if (args.size() > 0)
        {
            juce::String path = args[0].toString();
            juce::File f (path);
            if (f.existsAsFile())
            {
                juce::StringArray paths;
                paths.add (f.getFullPathName());
                
                juce::MessageManager::callAsync ([paths, this]
                {
                    juce::DragAndDropContainer::performExternalDragDropOfFiles (
                        paths, /*canMoveFiles*/ false, this);
                });
            }
        }
        completion (juce::var());
    });

    webView = std::make_unique<juce::WebBrowserComponent> (options);
    addAndMakeVisible (webView.get());

    webView->goToURL ("http://localhost/index.html");

    setSize (1000, 540);
}

ChopAudioProcessorEditor::~ChopAudioProcessorEditor()
{
}

void ChopAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111114));
}

void ChopAudioProcessorEditor::resized()
{
    if (webView != nullptr)
    {
        webView->setBounds (getLocalBounds());
    }
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
    processorRef.triggerLibraryScan();
}
