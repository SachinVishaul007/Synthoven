#pragma once

#include <juce_core/juce_core.h>

/**
 * In-memory representation of a backend SampleDto
 * (see backend/.../library/dto/SampleDto.java).
 */
struct Sample
{
    int            id        = 0;
    juce::String   name;
    juce::String   type;          // "LIBRARY" or "GENERATED"
    juce::String   format;        // wav, mp3, aiff, flac
    juce::int64    durationMs = 0;
    int            bpm        = 0; // 0 when unknown/null
    juce::String   musicalKey;
    juce::StringArray tags;
    bool           favorite   = false;
    juce::String   prompt;
    juce::String   provider;
    juce::String   localFilePath;

    bool isGenerated() const { return type.equalsIgnoreCase ("GENERATED"); }

    juce::String subtitle() const
    {
        if (isGenerated())
            return provider.isNotEmpty() ? (provider + " · generated") : "generated";
        return tags.joinIntoString (" ");
    }

    juce::String durationText() const
    {
        if (durationMs <= 0) return "--";
        if (durationMs >= 1000)
            return juce::String (durationMs / 1000.0, 2) + "s";
        return juce::String (durationMs) + "ms";
    }

    /** Parse one SampleDto JSON object. */
    static Sample fromVar (const juce::var& v)
    {
        Sample s;
        if (auto* o = v.getDynamicObject())
        {
            s.id         = (int) (juce::int64) o->getProperty ("id");
            s.name       = o->getProperty ("name").toString();
            s.type       = o->getProperty ("type").toString();
            s.format     = o->getProperty ("format").toString();
            s.durationMs  = (juce::int64) (double) o->getProperty ("durationMs");
            s.bpm        = (int) (juce::int64) o->getProperty ("bpm");
            s.musicalKey = o->getProperty ("musicalKey").toString();
            s.favorite   = (bool) o->getProperty ("favorite");
            s.prompt     = o->getProperty ("prompt").toString();
            s.provider   = o->getProperty ("provider").toString();

            auto tagsVar = o->getProperty ("tags");
            if (auto* arr = tagsVar.getArray())
                for (auto& t : *arr)
                    s.tags.add (t.toString());
        }
        return s;
    }

    /** Parse a JSON array of SampleDto objects. */
    static juce::Array<Sample> arrayFromVar (const juce::var& v)
    {
        juce::Array<Sample> out;
        if (auto* arr = v.getArray())
            for (auto& item : *arr)
                out.add (fromVar (item));
        return out;
    }
};
