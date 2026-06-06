#include "SampleLibrary.h"

SampleLibrary::SampleLibrary()
{
    libraryFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                      .getChildFile("Synthoven/Chop/library.json");
    libraryFile.getParentDirectory().createDirectory();
    load();
}

juce::String SampleLibrary::addSample(const juce::File& file)
{
    if (!file.existsAsFile())
        return {};

    Sample s    = extractMetadata(file);
    s.id        = generateId();
    s.addedAt   = juce::Time::currentTimeMillis();
    samples.add(s);
    save();
    return s.id;
}

bool SampleLibrary::deleteSample(const juce::String& id)
{
    for (int i = 0; i < samples.size(); ++i)
    {
        if (samples[i].id == id)
        {
            samples.remove(i);
            save();
            return true;
        }
    }
    return false;
}

bool SampleLibrary::updateSample(const juce::String& id, const juce::var& metadata)
{
    for (auto& s : samples)
    {
        if (s.id != id)
            continue;

        if (metadata.hasProperty("name")) s.name = metadata["name"].toString();
        if (metadata.hasProperty("pack")) s.pack = metadata["pack"].toString();
        if (metadata.hasProperty("tags"))
        {
            s.tags.clear();
            if (auto* arr = metadata["tags"].getArray())
                for (auto& t : *arr)
                    s.tags.add(t.toString());
        }
        save();
        return true;
    }
    return false;
}

juce::Array<Sample> SampleLibrary::getAllSamples() const
{
    return samples;
}

const Sample* SampleLibrary::getSample(const juce::String& id) const
{
    for (auto& s : samples)
        if (s.id == id)
            return &s;
    return nullptr;
}

juce::Array<Sample> SampleLibrary::search(const juce::String& query) const
{
    auto words = juce::StringArray::fromTokens(query.toLowerCase().trim(), " ", "");
    juce::Array<Sample> results;

    for (auto& s : samples)
    {
        for (auto& word : words)
        {
            if (s.name.toLowerCase().contains(word) ||
                s.pack.toLowerCase().contains(word))
            {
                results.add(s);
                break;
            }

            bool tagHit = false;
            for (auto& tag : s.tags)
                if (tag.toLowerCase().contains(word)) { tagHit = true; break; }

            if (tagHit) { results.add(s); break; }
        }
    }
    return results;
}

bool SampleLibrary::toggleFavorite(const juce::String& id)
{
    for (auto& s : samples)
    {
        if (s.id == id)
        {
            s.favorited = !s.favorited;
            save();
            return true;
        }
    }
    return false;
}

void SampleLibrary::load()
{
    if (!libraryFile.existsAsFile())
        return;

    auto parsed = juce::JSON::parse(libraryFile.loadFileAsString());
    if (auto* arr = parsed.getArray())
        for (auto& v : *arr)
            samples.add(Sample::fromVar(v));
}

void SampleLibrary::save() const
{
    juce::Array<juce::var> arr;
    for (auto& s : samples)
        arr.add(s.toVar());

    libraryFile.replaceWithText(juce::JSON::toString(juce::var(arr), true));
}

Sample SampleLibrary::extractMetadata(const juce::File& file) const
{
    Sample s;
    s.name    = file.getFileName();
    s.filePath = file.getFullPathName();
    s.pack    = file.getParentDirectory().getFileName();

    juce::AudioFormatManager fmt;
    fmt.registerBasicFormats();

    if (auto* reader = fmt.createReaderFor(file))
    {
        s.duration   = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
        s.sampleRate = static_cast<int>(reader->sampleRate);
        s.bitDepth   = reader->bitsPerSample;
        delete reader;
    }

    auto lower = s.name.toLowerCase();
    for (const char* kw : { "kick", "snare", "hat", "hihat", "bass", "pad",
                             "lead", "chord", "loop", "fx", "perc", "vocal" })
        if (lower.contains(kw))
            s.tags.add(kw);

    return s;
}

juce::String SampleLibrary::generateId()
{
    return juce::Uuid().toString().substring(0, 8);
}
