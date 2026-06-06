#pragma once
#include "Sample.h"
#include <JuceHeader.h>

class SampleLibrary {
public:
  SampleLibrary();

  // CRUD
  juce::String addSample(const juce::File &file);
  bool deleteSample(const juce::String &id);
  bool updateSample(const juce::String &id, const juce::var &metadata);
  juce::Array<Sample> getAllSamples() const;
  const Sample *getSample(const juce::String &id) const;

  // Search
  juce::Array<Sample> search(const juce::String &query) const;

  // Favorites
  bool toggleFavorite(const juce::String &id);

  // Directory scanning
  void scanDirectory(const juce::File &dir);

  // Persistence
  void load();
  void save() const;

private:
  static const juce::File defaultLibraryDir() {
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Chop")
        .getChildFile("Samples");
  }
  juce::Array<Sample> samples;
  juce::File libraryFile;

  Sample extractMetadata(const juce::File &file) const;
  static juce::String generateId();
};
