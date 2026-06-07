#include "ClapModelManager.h"
#include <cmath>
#include <numeric>

ClapModelManager::ClapModelManager()
{
}

ClapModelManager::~ClapModelManager()
{
    if (downloadThread != nullptr)
    {
        downloadThread->stopThread(4000);
        downloadThread.reset();
    }
    
    // Explicitly reset sessions before environment to prevent crash on exit
    textSession.reset();
    audioSession.reset();
    env.reset();
}

juce::File ClapModelManager::getModelsDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ChopSampleBrowser")
        .getChildFile("models");
}

void ClapModelManager::initialize()
{
    juce::File modelsDir = getModelsDirectory();
    
    juce::File vocabFile = modelsDir.getChildFile("vocab.json");
    juce::File mergesFile = modelsDir.getChildFile("merges.txt");
    juce::File textFile = modelsDir.getChildFile("text_model_quantized.onnx");
    juce::File audioFile = modelsDir.getChildFile("audio_model_quantized.onnx");

    if (vocabFile.exists() && mergesFile.exists() && textFile.exists() && audioFile.exists())
    {
        setStatus("Loading models...", 0.5f);
        if (loadModels())
        {
            setStatus("AI Search Ready", 1.0f);
        }
        else
        {
            setStatus("Failed to load models", 0.0f);
        }
    }
    else
    {
        // Start downloading in background
        downloadThread = std::make_unique<ModelDownloadThread>(*this);
        downloadThread->startThread();
    }
}

void ClapModelManager::setStatus(const juce::String& message, float progress)
{
    const juce::ScopedLock sl(statusLock);
    statusMessage = message;
    downloadProgress = progress;
}

juce::String ClapModelManager::getStatusMessage() const
{
    const juce::ScopedLock sl(statusLock);
    return statusMessage;
}

bool ClapModelManager::loadModels()
{
    try
    {
        juce::File modelsDir = getModelsDirectory();
        juce::File vocabFile = modelsDir.getChildFile("vocab.json");
        juce::File mergesFile = modelsDir.getChildFile("merges.txt");
        juce::File textFile = modelsDir.getChildFile("text_model_quantized.onnx");
        juce::File audioFile = modelsDir.getChildFile("audio_model_quantized.onnx");

        if (!tokenizer.load(vocabFile, mergesFile))
            return false;

        env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "CLAP");

        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(2);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

#ifdef _WIN32
        textSession = std::make_unique<Ort::Session>(*env, textFile.getFullPathName().toWideCharPointer(), sessionOptions);
        audioSession = std::make_unique<Ort::Session>(*env, audioFile.getFullPathName().toWideCharPointer(), sessionOptions);
#else
        textSession = std::make_unique<Ort::Session>(*env, textFile.getFullPathName().toRawUTF8(), sessionOptions);
        audioSession = std::make_unique<Ort::Session>(*env, audioFile.getFullPathName().toRawUTF8(), sessionOptions);
#endif

        querySessionTensorNames(*textSession, textInputNames, textOutputNames);
        querySessionTensorNames(*audioSession, audioInputNames, audioOutputNames);

        modelsLoaded = true;
        return true;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("ONNX Load Exception: " + juce::String(e.what()));
        modelsLoaded = false;
        return false;
    }
}

void ClapModelManager::querySessionTensorNames(const Ort::Session& session,
                                              std::vector<std::string>& inputNames,
                                              std::vector<std::string>& outputNames)
{
    inputNames.clear();
    outputNames.clear();
    Ort::AllocatorWithDefaultOptions allocator;

    size_t numInputs = session.GetInputCount();
    for (size_t i = 0; i < numInputs; ++i)
    {
        auto nameAllocated = session.GetInputNameAllocated(i, allocator);
        inputNames.push_back(std::string(nameAllocated.get()));
    }

    size_t numOutputs = session.GetOutputCount();
    for (size_t i = 0; i < numOutputs; ++i)
    {
        auto nameAllocated = session.GetOutputNameAllocated(i, allocator);
        outputNames.push_back(std::string(nameAllocated.get()));
    }
}

bool ClapModelManager::getTextEmbedding(const juce::String& text, std::vector<float>& outEmbedding)
{
    if (!modelsLoaded || textSession == nullptr)
        return false;

    try
    {
        std::vector<int64_t> inputIds64;
        std::vector<int64_t> attentionMask64;
        tokenizer.encode(text.toStdString(), 77, inputIds64, attentionMask64);

        std::vector<int64_t> inputDims = { 1, 77 }; // Batch size 1, Sequence length 77

        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

        std::vector<Ort::Value> inputs;
        std::vector<const char*> inputNamePtrs;

        for (const auto& name : textInputNames)
        {
            inputNamePtrs.push_back(name.c_str());
            if (name == "input_ids")
            {
                inputs.push_back(Ort::Value::CreateTensor<int64_t>(
                    memoryInfo, inputIds64.data(), inputIds64.size(), inputDims.data(), inputDims.size()));
            }
            else if (name == "attention_mask")
            {
                inputs.push_back(Ort::Value::CreateTensor<int64_t>(
                    memoryInfo, attentionMask64.data(), attentionMask64.size(), inputDims.data(), inputDims.size()));
            }
            else
            {
                // Fallback: feed attention mask or similar dummy data
                inputs.push_back(Ort::Value::CreateTensor<int64_t>(
                    memoryInfo, attentionMask64.data(), attentionMask64.size(), inputDims.data(), inputDims.size()));
            }
        }

        std::vector<const char*> outputNamePtrs;
        for (const auto& name : textOutputNames)
            outputNamePtrs.push_back(name.c_str());

        auto outputTensors = textSession->Run(
            Ort::RunOptions{nullptr},
            inputNamePtrs.data(),
            inputs.data(),
            inputs.size(),
            outputNamePtrs.data(),
            outputNamePtrs.size()
        );

        if (outputTensors.empty())
            return false;

        float* floatData = outputTensors[0].GetTensorMutableData<float>();
        outEmbedding.assign(floatData, floatData + 512);

        // Manually L2-normalize
        float norm = 0.0f;
        for (float val : outEmbedding)
            norm += val * val;
        norm = std::sqrt(norm);
        if (norm > 1e-6f)
        {
            for (float& val : outEmbedding)
                val /= norm;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("ONNX Text Inference Exception: " + juce::String(e.what()));
        return false;
    }
}

bool ClapModelManager::getAudioEmbedding(const std::vector<float>& spectrogram, std::vector<float>& outEmbedding)
{
    if (!modelsLoaded || audioSession == nullptr)
        return false;

    // Expected shape: [1, 1, 1001, 64]
    if (spectrogram.size() != 1001 * 64)
        return false;

    try
    {
        std::vector<int64_t> featuresDims = { 1, 1, 1001, 64 };
        std::vector<int64_t> isLongerDims = { 1, 1 };
        bool isLongerVal = false;

        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

        std::vector<Ort::Value> inputs;
        std::vector<const char*> inputNamePtrs;

        for (const auto& name : audioInputNames)
        {
            inputNamePtrs.push_back(name.c_str());
            if (name == "input_features")
            {
                inputs.push_back(Ort::Value::CreateTensor<float>(
                    memoryInfo, const_cast<float*>(spectrogram.data()), spectrogram.size(), featuresDims.data(), featuresDims.size()));
            }
            else if (name == "is_longer")
            {
                inputs.push_back(Ort::Value::CreateTensor<bool>(
                    memoryInfo, &isLongerVal, 1, isLongerDims.data(), isLongerDims.size()));
            }
            else
            {
                // Fallback dummy float tensor
                inputs.push_back(Ort::Value::CreateTensor<float>(
                    memoryInfo, const_cast<float*>(spectrogram.data()), spectrogram.size(), featuresDims.data(), featuresDims.size()));
            }
        }

        std::vector<const char*> outputNamePtrs;
        for (const auto& name : audioOutputNames)
            outputNamePtrs.push_back(name.c_str());

        auto outputTensors = audioSession->Run(
            Ort::RunOptions{nullptr},
            inputNamePtrs.data(),
            inputs.data(),
            inputs.size(),
            outputNamePtrs.data(),
            outputNamePtrs.size()
        );

        if (outputTensors.empty())
            return false;

        float* floatData = outputTensors[0].GetTensorMutableData<float>();
        outEmbedding.assign(floatData, floatData + 512);

        // Manually L2-normalize
        float norm = 0.0f;
        for (float val : outEmbedding)
            norm += val * val;
        norm = std::sqrt(norm);
        if (norm > 1e-6f)
        {
            for (float& val : outEmbedding)
                val /= norm;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("ONNX Audio Inference Exception: " + juce::String(e.what()));
        return false;
    }
}

float ClapModelManager::computeSimilarity(const std::vector<float>& vecA, const std::vector<float>& vecB) const
{
    if (vecA.size() != vecB.size() || vecA.empty())
        return 0.0f;

    return std::inner_product(vecA.begin(), vecA.end(), vecB.begin(), 0.0f);
}

void ClapModelManager::ModelDownloadThread::run()
{
    owner.downloadInProgress = true;
    owner.downloadProgress = 0.0f;

    juce::File modelsDir = owner.getModelsDirectory();
    if (!modelsDir.exists())
        modelsDir.createDirectory();

    struct DownloadItem
    {
        juce::String url;
        juce::File localFile;
        float progressWeight;
    };

    std::vector<DownloadItem> items = {
        { "https://huggingface.co/Xenova/clap-htsat-unfused/resolve/main/vocab.json", modelsDir.getChildFile("vocab.json"), 0.05f },
        { "https://huggingface.co/Xenova/clap-htsat-unfused/resolve/main/merges.txt", modelsDir.getChildFile("merges.txt"), 0.05f },
        { "https://huggingface.co/Xenova/clap-htsat-unfused/resolve/main/onnx/text_model_quantized.onnx", modelsDir.getChildFile("text_model_quantized.onnx"), 0.35f },
        { "https://huggingface.co/Xenova/clap-htsat-unfused/resolve/main/onnx/audio_model_quantized.onnx", modelsDir.getChildFile("audio_model_quantized.onnx"), 0.55f }
    };

    float currentProgress = 0.0f;
    bool allSucceeded = true;

    for (const auto& item : items)
    {
        if (threadShouldExit())
        {
            allSucceeded = false;
            break;
        }

        // If file already exists and is not empty, we assume it's downloaded
        if (item.localFile.exists() && item.localFile.getSize() > 1024)
        {
            currentProgress += item.progressWeight;
            owner.setStatus("Checking files...", currentProgress);
            continue;
        }

        owner.setStatus("Downloading " + item.localFile.getFileName() + "...", currentProgress);

        juce::URL url(item.url);
        juce::URL::DownloadTaskOptions options;
        
        auto task = url.downloadToFile(item.localFile, options);
        if (task == nullptr)
        {
            allSucceeded = false;
            break;
        }

        while (!task->isFinished())
        {
            if (threadShouldExit())
            {
                task.reset(); // Cancel download
                allSucceeded = false;
                break;
            }

            float itemProgress = 0.0f;
            if (task->getTotalLength() > 0)
            {
                itemProgress = (float)task->getLengthDownloaded() / (float)task->getTotalLength();
            }

            owner.setStatus("Downloading " + item.localFile.getFileName() + " (" + juce::String(int(itemProgress * 100)) + "%)...",
                            currentProgress + itemProgress * item.progressWeight);

            juce::Thread::sleep(100);
        }

        if (task->hadError() || !item.localFile.exists() || item.localFile.getSize() == 0)
        {
            item.localFile.deleteFile();
            allSucceeded = false;
            break;
        }

        currentProgress += item.progressWeight;
        owner.setStatus("Downloading...", currentProgress);
    }

    owner.downloadInProgress = false;

    if (allSucceeded)
    {
        owner.setStatus("Loading models...", 1.0f);
        if (owner.loadModels())
        {
            owner.setStatus("AI Search Ready", 1.0f);
        }
        else
        {
            owner.setStatus("Failed to load models", 0.0f);
        }
    }
    else
    {
        owner.setStatus("Model download failed. Please refresh to try again.", 0.0f);
    }
}
