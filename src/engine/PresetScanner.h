// Copyright (C) 2025 tilr
#pragma once
#include <JuceHeader.h>
#include <functional>
#include "PresetManager.h"

class PresetScanner : public juce::Thread
{
public:
    PresetScanner(const juce::File& dir,
        std::function<void()> callback,
        std::unordered_map<String, PresetManager::Preset>& cache,
        const juce::String& pattern = "*.xml;*.preset")
        : Thread("PresetScanThread"),
        dir(dir),
        filePattern(pattern),
        onComplete(callback),
        cache(cache)
    {
    }

    void run() override
    {
        scanFolder(dir);

        if (!threadShouldExit() && onComplete) {
            juce::MessageManager::callAsync([cb = onComplete]()
                {
                    cb();
                });
        }
    }

private:
    std::unordered_map<String, PresetManager::Preset>& cache;
    juce::File dir;
    juce::String filePattern;
    std::function<void()> onComplete;

    void scanFolder(const juce::File& folder);
    void readPresetFile(File file);
};