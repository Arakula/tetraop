#include "PresetScanner.h"

void PresetScanner::scanFolder(const juce::File& folder)
{
    if (threadShouldExit() || !folder.exists() || !folder.isDirectory())
        return;

    juce::Array<juce::File> children;
    folder.findChildFiles(children, juce::File::findFiles, false, filePattern);

    for (auto& file : children) {
        auto& path = file.getFullPathName();
        if (!cache.count(path)) {
            readPresetFile(file);
        }
        else {
            auto date = file.getLastModificationTime().toMilliseconds();
            if (date > cache[path].fdate) {
                readPresetFile(file);
            }
            cache[path].exists = true;
        }
    }

    // Recurse into subfolders
    folder.findChildFiles(children, juce::File::findDirectories, false);
    for (auto& sub : children)
        scanFolder(sub);
}

void PresetScanner::readPresetFile(File file)
{
    if (!file.existsAsFile() || file.isDirectory())
        return;


    std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
        return;

    PresetManager::Preset preset;
    auto& oldEntry = cache[file.getFullPathName()]; // preset may already exist in cache if it was modified

    String path = file.getFullPathName();
    preset.version = xml->getStringAttribute("version", PROJECT_VERSION);
    preset.path = path;
    preset.fdate = file.getLastModificationTime().toMilliseconds();
    preset.exists = true;

    preset.id = xml->getStringAttribute("id", juce::Uuid().toString());
    preset.name = xml->getStringAttribute("name", file.getFileNameWithoutExtension());
    preset.description = xml->getStringAttribute("description");
    preset.category = xml->getStringAttribute("category");
    preset.author = xml->getStringAttribute("author");
    preset.date = juce::Time::fromISO8601(xml->getStringAttribute("date")).toMilliseconds();
    preset.liked = oldEntry.liked;

    auto it = cache.find(path);
    if (it == cache.end()) {
        cache.emplace(path, std::move(preset));
    }
    else {
        it->second = std::move(preset);
    }
}