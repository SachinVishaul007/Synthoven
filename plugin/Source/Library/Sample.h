#pragma once
#include <JuceHeader.h>

struct Sample
{
    juce::String     id;
    juce::String     name;
    juce::String     filePath;
    juce::String     pack;
    juce::StringArray tags;
    double           duration   { 0.0 };
    int              sampleRate { 0 };
    int              bitDepth   { 0 };
    bool             favorited  { false };
    juce::int64      addedAt    { 0 };

    juce::var toVar() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("id",         id);
        obj->setProperty("name",       name);
        obj->setProperty("filePath",   filePath);
        obj->setProperty("pack",       pack);
        obj->setProperty("duration",   duration);
        obj->setProperty("sampleRate", sampleRate);
        obj->setProperty("bitDepth",   bitDepth);
        obj->setProperty("favorited",  favorited);
        obj->setProperty("addedAt",    addedAt);
        obj->setProperty("type",       juce::String("library"));

        juce::Array<juce::var> tagsArr;
        for (auto& t : tags)
            tagsArr.add(t);
        obj->setProperty("tags", tagsArr);

        return juce::var(obj);
    }

    static Sample fromVar(const juce::var& v)
    {
        Sample s;
        s.id         = v["id"].toString();
        s.name       = v["name"].toString();
        s.filePath   = v["filePath"].toString();
        s.pack       = v["pack"].toString();
        s.duration   = static_cast<double>(v["duration"]);
        s.sampleRate = static_cast<int>(v["sampleRate"]);
        s.bitDepth   = static_cast<int>(v["bitDepth"]);
        s.favorited  = static_cast<bool>(v["favorited"]);
        s.addedAt    = static_cast<juce::int64>(v["addedAt"]);

        if (auto* arr = v["tags"].getArray())
            for (auto& t : *arr)
                s.tags.add(t.toString());

        return s;
    }
};
