#include "JSBridge.h"

JSBridge::JSBridge(SampleLibrary& lib, SamplePlayer& p)
    : library(lib), player(p)
{}

juce::var JSBridge::handleMessage(const juce::String& action, const juce::var& payload)
{
    if (action == "getSamples")       return handleGetSamples(payload);
    if (action == "search")           return handleSearch(payload);
    if (action == "addSample")        return handleAddSample(payload);
    if (action == "deleteSample")     return handleDeleteSample(payload);
    if (action == "updateSample")     return handleUpdateSample(payload);
    if (action == "upload")           return handleUpload(payload);
    if (action == "play")             return handlePlay(payload);
    if (action == "stop")             return handleStop(payload);
    if (action == "toggleFavorite")   return handleToggleFavorite(payload);
    if (action == "getPlaybackState") return handleGetPlaybackState(payload);

    return err("Unknown action: " + action);
}

// ---------------------------------------------------------------------------

juce::var JSBridge::handleGetSamples(const juce::var&)
{
    juce::Array<juce::var> arr;
    for (auto& s : library.getAllSamples())
        arr.add(s.toVar());
    return ok(arr);
}

juce::var JSBridge::handleSearch(const juce::var& payload)
{
    juce::Array<juce::var> arr;
    for (auto& s : library.search(payload["query"].toString()))
        arr.add(s.toVar());
    return ok(arr);
}

juce::var JSBridge::handleAddSample(const juce::var& payload)
{
    auto id = library.addSample(juce::File(payload["filePath"].toString()));
    return id.isNotEmpty() ? ok(id) : err("File not found or unsupported format");
}

juce::var JSBridge::handleDeleteSample(const juce::var& payload)
{
    return library.deleteSample(payload["id"].toString())
               ? ok()
               : err("Sample not found");
}

juce::var JSBridge::handleUpdateSample(const juce::var& payload)
{
    return library.updateSample(payload["id"].toString(), payload["metadata"])
               ? ok()
               : err("Sample not found");
}

juce::var JSBridge::handleUpload(const juce::var& payload)
{
    auto destDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("Synthoven/Chop/uploads");
    destDir.createDirectory();

    auto destFile = destDir.getChildFile(payload["fileName"].toString());
    juce::MemoryOutputStream decoded;
    juce::Base64::convertFromBase64(decoded, payload["data"].toString());
    destFile.replaceWithData(decoded.getData(), decoded.getDataSize());

    auto id = library.addSample(destFile);
    return id.isNotEmpty() ? ok(id) : err("Failed to register uploaded file");
}

juce::var JSBridge::handlePlay(const juce::var& payload)
{
    auto* sample = library.getSample(payload["id"].toString());
    if (sample == nullptr)
        return err("Sample not found");

    if (!player.loadFile(juce::File(sample->filePath)))
        return err("Could not load audio file");

    player.play();
    return ok();
}

juce::var JSBridge::handleStop(const juce::var&)
{
    player.stop();
    return ok();
}

juce::var JSBridge::handleToggleFavorite(const juce::var& payload)
{
    return library.toggleFavorite(payload["id"].toString())
               ? ok()
               : err("Sample not found");
}

juce::var JSBridge::handleGetPlaybackState(const juce::var&)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("isPlaying", player.isPlaying());
    obj->setProperty("position",  player.getPositionSeconds());
    obj->setProperty("duration",  player.getDurationSeconds());
    return ok(juce::var(obj));
}

// ---------------------------------------------------------------------------

juce::var JSBridge::ok(const juce::var& data)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("ok", true);
    if (!data.isVoid())
        obj->setProperty("data", data);
    return juce::var(obj);
}

juce::var JSBridge::err(const juce::String& message)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("ok",    false);
    obj->setProperty("error", message);
    return juce::var(obj);
}
