#include "PluginProcessor.h"
#include "PluginEditor.h"

ChopAudioProcessor::ChopAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats(); // WAV + AIFF always; FLAC/Ogg if enabled in the build
    readAheadThread.startThread();
    loadEmbeddingCache();
    modelManager.initialize();
}

ChopAudioProcessor::~ChopAudioProcessor()
{
    saveEmbeddingCache();
    stopAudition();
    transport.setSource (nullptr);
    readAheadThread.stopThread (2000);
}

void ChopAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    transport.prepareToPlay (samplesPerBlock, sampleRate);
}

void ChopAudioProcessor::releaseResources()
{
    transport.releaseResources();
}

bool ChopAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    const auto in = layouts.getMainInputChannelSet();
    if (! in.isDisabled() && in != out)
        return false;

    return true;
}

void ChopAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    juce::AudioSourceChannelInfo info (&buffer, 0, buffer.getNumSamples());
    transport.getNextAudioBlock (info); // fills with audition audio (silence when stopped)

    // A mono source only fills channel 0 — mirror it to the right channel.
    if (buffer.getNumChannels() >= 2 && numSourceChannels.load() == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, buffer.getNumSamples());
}

void ChopAudioProcessor::auditionFile (const juce::File& file)
{
    stopAudition();

    auto* reader = formatManager.createReaderFor (file);
    if (reader != nullptr)
    {
        numSourceChannels.store (reader->numChannels);
        readerSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
        transport.setSource (readerSource.get(), 32768, &readAheadThread, reader->sampleRate);
        transport.setPosition (0.0);
        transport.start();
    }
}

void ChopAudioProcessor::stopAudition()
{
    transport.stop();
    transport.setSource (nullptr);
    readerSource.reset();
}

juce::AudioProcessorEditor* ChopAudioProcessor::createEditor()
{
    return new ChopAudioProcessorEditor (*this);
}

void ChopAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml ("CHOPSETTINGS");
    xml.setAttribute ("libraryFolder", libraryFolder.getFullPathName());
    copyXmlToBinary (xml, destData);
}

void ChopAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName ("CHOPSETTINGS"))
        {
            libraryFolder = juce::File (xmlState->getStringAttribute ("libraryFolder"));
            triggerLibraryScan();
        }
    }
}

void ChopAudioProcessor::setLibraryFolder (const juce::File& folder)
{
    libraryFolder = folder;
    triggerLibraryScan();
}

void ChopAudioProcessor::triggerLibraryScan()
{
    if (libraryFolder.isDirectory())
        scannerThread.startScan (libraryFolder);
}

void ChopAudioProcessor::setScannedFiles (juce::Array<juce::File> files)
{
    const juce::ScopedLock sl (localFilesLock);
    localLibraryFiles = std::move (files);
}

juce::Array<juce::File> ChopAudioProcessor::getScannedFiles() const
{
    const juce::ScopedLock sl (localFilesLock);
    return localLibraryFiles;
}

// ── Embedding Caching and ML Search Implementations ───────────────────────

juce::File ChopAudioProcessor::getEmbeddingCacheFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("ChopSampleBrowser")
        .getChildFile ("embedding_cache.bin");
}

void ChopAudioProcessor::loadEmbeddingCache()
{
    juce::File file = getEmbeddingCacheFile();
    if (! file.exists())
        return;

    juce::FileInputStream is (file);
    if (! is.openedOk())
        return;

    char magic[8];
    if (is.read (magic, 8) != 8 || std::memcmp (magic, "CHOPCLAP", 8) != 0)
        return;

    int version = is.readInt();
    if (version != 1)
        return;

    const juce::ScopedLock sl (cacheLock);
    embeddingCache.clear();

    int count = is.readInt();
    for (int i = 0; i < count; ++i)
    {
        juce::String relPath = is.readString();
        juce::int64 modTime = is.readInt64();
        
        std::vector<float> embedding (512);
        int bytesRead = is.read (embedding.data(), (int)(sizeof (float) * 512));
        if (bytesRead == (int)(sizeof (float) * 512))
        {
            embeddingCache[relPath] = { modTime, std::move (embedding) };
        }
    }
}

void ChopAudioProcessor::saveEmbeddingCache()
{
    juce::File file = getEmbeddingCacheFile();
    if (! file.getParentDirectory().exists())
        file.getParentDirectory().createDirectory();

    juce::TemporaryFile tempFile (file);
    {
        juce::FileOutputStream os (tempFile.getFile());
        if (os.openedOk())
        {
            os.write ("CHOPCLAP", 8);
            os.writeInt (1); // version
            
            const juce::ScopedLock sl (cacheLock);
            os.writeInt ((int)embeddingCache.size());

            for (const auto& pair : embeddingCache)
            {
                os.writeString (pair.first);
                os.writeInt64 (pair.second.lastModifiedTime);
                os.write (pair.second.embedding.data(), (int)(sizeof (float) * 512));
            }
        }
    }
    
    if (tempFile.getFile().exists())
    {
        tempFile.overwriteTargetFileWithTemporary();
    }
}

juce::String ChopAudioProcessor::normalizePath (const juce::String& path) const
{
    return path.replaceCharacter ('\\', '/').toLowerCase();
}

juce::String ChopAudioProcessor::getRelativePath (const juce::File& file, const juce::File& root) const
{
    return normalizePath (file.getRelativePathFrom (root));
}

juce::Array<juce::File> ChopAudioProcessor::performSearch (const juce::String& query)
{
    juce::Array<juce::File> results;
    auto allFiles = getScannedFiles();
    if (allFiles.isEmpty())
        return results;

    juce::String trimmedQuery = query.trim().toLowerCase();
    if (trimmedQuery.isEmpty())
        return allFiles;

    // Check if semantic search is enabled and model is loaded
    bool useSemantic = semanticSearchEnabled.load() && modelManager.isModelLoaded();
    std::vector<float> queryEmbedding;
    if (useSemantic)
    {
        useSemantic = modelManager.getTextEmbedding (query, queryEmbedding);
    }

    struct SearchResult
    {
        juce::File file;
        float score = 0.0f;
    };
    std::vector<SearchResult> candidates;

    // Split query into words for keyword matching fallback
    juce::StringArray queryWords;
    queryWords.addTokens (trimmedQuery, " -_", "");
    queryWords.removeEmptyStrings();

    for (const auto& file : allFiles)
    {
        juce::String filename = file.getFileNameWithoutExtension().toLowerCase();
        
        // 1. Keyword scoring
        float keywordScore = 0.0f;
        if (filename.contains (trimmedQuery))
        {
            keywordScore = 1.0f;
        }
        else if (queryWords.size() > 0)
        {
            int matchingWords = 0;
            for (const auto& word : queryWords)
            {
                if (filename.contains (word))
                    matchingWords++;
            }
            if (matchingWords > 0)
                keywordScore = 0.5f * ((float)matchingWords / (float)queryWords.size());
        }

        // 2. Semantic scoring
        float similarity = 0.0f;
        bool hasEmbedding = false;
        if (useSemantic)
        {
            juce::String relPath = getRelativePath (file, libraryFolder);
            const juce::ScopedLock sl (cacheLock);
            auto it = embeddingCache.find (relPath);
            if (it != embeddingCache.end())
            {
                similarity = modelManager.computeSimilarity (queryEmbedding, it->second.embedding);
                hasEmbedding = true;
            }
        }

        // 3. Combined score
        float finalScore = 0.0f;
        if (useSemantic && hasEmbedding)
        {
            // Cosine similarity is usually positive for text-audio matches in CLAP.
            // We combine similarity with a keyword match boost.
            finalScore = similarity + keywordScore * 0.3f;
            
            // Only keep if similarity indicates some relevance, or keyword matches.
            if (similarity > 0.12f || keywordScore > 0.0f)
            {
                candidates.push_back ({ file, finalScore });
            }
        }
        else
        {
            // Fall back to keyword search only
            finalScore = keywordScore;
            if (keywordScore > 0.0f)
            {
                candidates.push_back ({ file, finalScore });
            }
        }
    }

    // Sort in descending order of score
    std::sort (candidates.begin(), candidates.end(), [] (const SearchResult& a, const SearchResult& b) {
        return a.score > b.score;
    });

    for (const auto& cand : candidates)
    {
        results.add (cand.file);
    }

    return results;
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChopAudioProcessor();
}
