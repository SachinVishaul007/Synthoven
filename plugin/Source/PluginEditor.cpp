#include "PluginEditor.h"

ChopEditor::ChopEditor(ChopProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      bridge(library, p.getSamplePlayer()),
      webView(juce::WebBrowserComponent::Options{}
          .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
          .withWinWebView2Options(
              juce::WebBrowserComponent::Options::WinWebView2{}
                  .withUserDataFolder(
                      juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getChildFile("ChopWebView2")))
          .withNativeFunction(
              "chopSend",
              [this](const juce::Array<juce::var>& args,
                     juce::WebBrowserComponent::NativeFunctionCompletion complete)
              {
                  if (args.size() < 2) { complete(juce::var("invalid args")); return; }

                  auto action = args[0].toString();

                  if (action == "browseForFiles")
                  {
                      handleBrowseForFiles(std::move(complete));
                      return;
                  }

                  if (action == "selectLibraryFolder")
                  {
                      handleSelectLibraryFolder(std::move(complete));
                      return;
                  }

                  complete(bridge.handleMessage(action, args[1]));
              }))
{
    setSize(900, 600);
    addAndMakeVisible(webView);
    webView.goToURL("http://localhost:5173");

#if JUCE_DEBUG
    webView.evaluateJavascript(
        "window.__chopDebug = async (action, payload) => {"
        "  const fn = window.__JUCE__.backend.getNativeFunction('chopSend');"
        "  const res = await fn(action, payload ?? {});"
        "  console.log('[Chop]', action, res);"
        "  return res;"
        "};",
        nullptr);
#endif
}

ChopEditor::~ChopEditor() {}

void ChopEditor::resized()
{
    webView.setBounds(getLocalBounds());
}

void ChopEditor::handleBrowseForFiles(juce::WebBrowserComponent::NativeFunctionCompletion complete)
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select audio samples",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aif;*.aiff;*.flac;*.mp3");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectMultipleItems,
        [this, complete = std::move(complete)](const juce::FileChooser& fc)
        {
            juce::Array<juce::var> added;
            for (auto& file : fc.getResults())
            {
                auto id = library.addSample(file);
                if (id.isNotEmpty())
                    if (auto* s = library.getSample(id))
                        added.add(s->toVar());
            }

            auto* obj = new juce::DynamicObject();
            obj->setProperty("ok", true);
            obj->setProperty("data", added);
            complete(juce::var(obj));
        });
}

void ChopEditor::handleSelectLibraryFolder(juce::WebBrowserComponent::NativeFunctionCompletion complete)
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select library folder",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        juce::String());

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this, complete = std::move(complete)](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.isDirectory())
            {
                library.clear();
                library.scanDirectory(result);

                juce::Array<juce::var> added;
                for (auto& s : library.getAllSamples())
                    added.add(s.toVar());

                auto* obj = new juce::DynamicObject();
                obj->setProperty("ok", true);
                obj->setProperty("data", added);
                complete(juce::var(obj));
            }
            else
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty("ok", false);
                obj->setProperty("error", "No folder selected");
                complete(juce::var(obj));
            }
        });
}
