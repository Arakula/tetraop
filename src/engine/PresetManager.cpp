#include "PresetManager.h"
#include "PresetScanner.h"
#include "../PluginProcessor.h"

PresetManager::PresetManager(TetraOPAudioProcessor& audioProcessor, String dir)
    : audioProcessor(audioProcessor)
    , dir(dir)
{
	if (!File(dir).exists())
    	File(dir).createDirectory();

    userDir = File(dir).getChildFile("User");
    factoryDir = File(dir).getChildFile("Factory");

    if (!userDir.exists()) userDir.createDirectory();
    if (!factoryDir.exists()) factoryDir.createDirectory();

    setSelected({});
	//loadCache();
    //scan();
}

PresetManager::~PresetManager()
{
}

void PresetManager::scan()
{
    if (scanner != nullptr) return;

    for (auto& [path, preset] : cache)
        preset.exists = false;

    scanner = std::make_unique<PresetScanner>(dir, [this]()
        {
            onScanComplete();
        }, cache, "*.xml");
    scanner->startThread();
}

void PresetManager::onScanComplete()
{
    String initPath = File(dir) // for compatibility purposes
        .getChildFile("Factory")
        .getChildFile("Init.xml")
        .getFullPathName();

    for (auto it = cache.begin(); it != cache.end(); ) {
        auto test = it->second;
        if (!it->second.exists && it->second.path != initPath)
            it = cache.erase(it);
        else
            ++it;
    }

    Preset init;
    init.version = PROJECT_VERSION;
    init.id = "";
    init.path = initPath;
    init.fdate = 0;
    init.date = 0;
    init.name = "-- Init --";
    init.description = "";
    init.category = "Templates";
    init.author = "";
    init.liked = cache.count(init.path) ? cache[init.path].liked : false;
    cache[init.path] = init;

    // update bank names for each preset
    // maybe this should be done elsewhere
    for (auto& [path, preset] : cache) {
        preset.bank = getPresetBank(preset.path);
    }

    scanner.reset();
    saveCache();
    UIDirty.store(true);
}

void PresetManager::saveCache()
{
    juce::Array<juce::var> jsonPresets;

    for (auto& [path, preset] : cache)
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("version", preset.version);
        obj->setProperty("id", preset.id);
        obj->setProperty("path", preset.path);
        obj->setProperty("fdate", juce::Time(preset.fdate).toISO8601(false));
        obj->setProperty("date", juce::Time(preset.date).toISO8601(false));
        obj->setProperty("name", preset.name);
        obj->setProperty("description", preset.description);
        obj->setProperty("category", preset.category);
        obj->setProperty("author", preset.author);
        obj->setProperty("liked", preset.liked);

        jsonPresets.add(obj.get());
    }

    juce::var rootVar(jsonPresets);
    juce::String jsonString = juce::JSON::toString(rootVar, false);
    juce::File file = juce::File(dir).getChildFile("presets.json");
    file.replaceWithText(jsonString);
}


void PresetManager::loadCache()
{
    juce::File file = juce::File(dir).getChildFile("presets.json");
    if (!file.existsAsFile())
        return;

    juce::String jsonString = file.loadFileAsString();
    juce::var rootVar = juce::JSON::parse(jsonString);
    if (!rootVar.isArray())
        return;

    auto* arr = rootVar.getArray();
    if (!arr)
        return;

    cache.clear();

    for (auto& item : *arr)
    {
        if (!item.isObject())
            continue;

        auto* obj = item.getDynamicObject();
        if (!obj)
            continue;

        Preset preset;
        preset.version = obj->getProperty("version").toString();
        preset.id       = obj->getProperty("id").toString();
        preset.path     = obj->getProperty("path").toString();
        preset.fdate    = juce::Time::fromISO8601(obj->getProperty("fdate").toString()).toMilliseconds();
        preset.date    = juce::Time::fromISO8601(obj->getProperty("date").toString()).toMilliseconds();
        preset.name     = obj->getProperty("name").toString();
        preset.description     = obj->getProperty("description").toString();
        preset.category = obj->getProperty("category").toString();
        preset.liked = obj->getProperty("liked");
        preset.author   = obj->getProperty("author").toString();

        cache[preset.path] = std::move(preset);
    }
}

void PresetManager::savePreset(
    String name,
    String description,
    String category,
    String author,
    bool overrideByName
) {
    if (name.isEmpty()) return;

    juce::MemoryBlock state;
    audioProcessor.getStateInformation(state);
    std::unique_ptr<juce::XmlElement> xml(
        audioProcessor.getXmlFromBinary(state.getData(), (int)state.getSize())
    );

    if (xml == nullptr) {
        return;
    }

    auto fname = File::createLegalFileName(name);
    File file = File(dir).getChildFile("User").getChildFile(fname).withFileExtension("xml");

    // attempts to save over a file in users directory with the same preset name
    // means that presets are identified by preset name rather than file name
    if (overrideByName) {
        auto path = getUserPresetPath(name);
        if (path.isNotEmpty()) {
            file = File(path);
        }
    }

    auto id = Uuid().toString();
    xml->setAttribute("version", PROJECT_VERSION);
    xml->setAttribute("id", id);
    xml->setAttribute("name", name.trim());
    xml->setAttribute("description", description);
    xml->setAttribute("category", category);
    xml->setAttribute("author", author);
    xml->setAttribute("date", Time::getCurrentTime().toISO8601(false));

    file.replaceWithText(xml->toString());

    Preset preset;
    preset.name = name.trim();
    preset.id = id;
    preset.path = file.getFullPathName();

    loadPreset(preset);
    scan();
}

void PresetManager::loadPreset(Preset preset)
{
    isLoadingPreset = true;
    String path = preset.path;
    File file = File(path);
    if (!file.existsAsFile() || file.isDirectory()) {
        loadInit();
        return;
    }

    std::unique_ptr<juce::XmlElement> xml;
    xml = juce::XmlDocument::parse(file); // load preset file

    if (xml == nullptr) {
        loadInit();
        return;
    }

    try {
        juce::MemoryBlock state;
        audioProcessor.copyXmlToBinary(*xml, state);
        audioProcessor.setStateInformation(state.getData(), (int)state.getSize()); // sets selected preset from setStateInformation()
    }
    catch (...) {
        loadInit();
        return;
    }
    isLoadingPreset = false;
}

void PresetManager::loadPresetFromPath(String path)
{
    Preset p;
    p.path = path;
    loadPreset(p);
}

void PresetManager::loadInit()
{
    isLoadingPreset = true;
     audioProcessor.setStateInformation(BinaryData::Init_xml, BinaryData::Init_xmlSize);
    isLoadingPreset = false;
}

String PresetManager::exportPreset(Preset preset)
{
    juce::MemoryBlock state;
    audioProcessor.getStateInformation(state);
    std::unique_ptr<juce::XmlElement> xml(
        audioProcessor.getXmlFromBinary(state.getData(), (int)state.getSize())
    );

    if (xml == nullptr) {
        return "";
    }

    xml->setAttribute("version", PROJECT_VERSION);
    xml->setAttribute("id", Uuid().toString());
    xml->setAttribute("name", preset.name);
    xml->setAttribute("description", preset.description);
    xml->setAttribute("category", preset.category);
    xml->setAttribute("author", preset.author);
    xml->setAttribute("date", Time::getCurrentTime().toISO8601(false));

    return xml->toString();
}

String PresetManager::importPreset(File file)
{
    auto fname = file.getFileNameWithoutExtension();
    auto ext = file.getFileExtension();
    File newfile;
    int i = 0;
    do {
        auto suffix = i == 0 ? "" : String(" (") + String(i) + ")";
        newfile = File(dir).getChildFile("User").getChildFile(fname + suffix + ext);
        i += 1;
    } while (newfile.existsAsFile());

    std::unique_ptr<juce::XmlElement> xml = XmlDocument::parse(file);
    if (xml == nullptr) {
        return "";
    }

    xml->setAttribute("id", Uuid().toString()); // avoid files with same ID
    newfile.replaceWithText(xml->toString());
    return newfile.getFullPathName();
}

void PresetManager::removePreset(Preset p)
{
    if (File(p.path).existsAsFile()) {
        File(p.path).deleteFile();
    }
    if (cache.count(p.path)) {
        cache.erase(p.path);
        saveCache();
    }
    if (selectedPreset.id == p.id) {
        loadInit();
    }
    UIDirty.store(true);
}

void PresetManager::setSelected(Preset preset)
{
    if (preset.id.isEmpty()) {
        preset.version = PROJECT_VERSION;
        preset.name = "-- Init --";
        preset.path = "";
    }

    selectedPreset = preset;
    nameDirty.store(true);
    selectedDirty.store(true);
}

void PresetManager::setBank(String bank, String category, String _sort, bool _sortAsc, bool _liked)
{
    selectedBank = bank;
    selectedCategory = category;
    sort = _sort;
    sortAsc = _sortAsc;
    liked = _liked;
    UIDirty.store(true);
}

std::vector<PresetManager::Bank>
PresetManager::getBanks()
{
    std::unordered_map<String, Bank> banks;
    for (const auto& [path, preset] : cache) {
        auto relative = File(path).getRelativePathFrom(dir);

        juce::StringArray parts;
        parts.addTokens(relative, "\\/", ""); // split by / or \

        if (parts.size() > 1) {
            auto& bankname = parts[0];
            if (!banks.count(bankname)) {
                banks[bankname].id = bankname;
            }
            if (preset.category.isNotEmpty()) {
                banks[bankname].categories.insert(preset.category);
            }
        }
    }

    std::vector<Bank> out;
    for (auto& [id, bank] : banks) {
        out.push_back(bank);
    }

    std::sort(out.begin(), out.end(), [](const Bank& a, const Bank& b)
        {
            return a.id < b.id;
        });

    return out;
}

std::vector<PresetManager::Preset>
PresetManager::getPresets()
{
    String bankDir = "";
    if (selectedBank.isNotEmpty())
        bankDir = File(dir).getChildFile(selectedBank).getFullPathName();

    std::vector<Preset> out;
    for (auto& [key, preset] : cache) {
        if (bankDir.isEmpty() || key.startsWith(bankDir)) {
            out.push_back(preset);
        }
    }

    if (selectedCategory.isNotEmpty()) {
        auto it = std::remove_if(out.begin(), out.end(),
            [this](const Preset& p) {
                return p.category != selectedCategory;
            });
        out.erase(it, out.end());
    }

    if (liked) {
        auto it = std::remove_if(out.begin(), out.end(),
            [this](const Preset& p) {
                return !p.liked;
            });
        out.erase(it, out.end());
    }

    if (sort == "category") {
        std::sort(out.begin(), out.end(),
            [this](const Preset& a, const Preset& b) {
                if (a.category == b.category) return a.name < b.name ;
                return sortAsc ? a.category < b.category : a.category > b.category;
            });
    }
    else if (sort == "bank") {
        std::sort(out.begin(), out.end(),
            [this](const Preset& a, const Preset& b) {
                if (a.bank == b.bank) return a.name < b.name;
                return sortAsc ? a.bank < b.bank : a.bank > b.bank;
            });
    }
    else if (sort == "author") {
        std::sort(out.begin(), out.end(),
            [this](const Preset& a, const Preset& b) {
                if (a.author == b.author) return a.name < b.name;
                return sortAsc ? a.author < b.author : a.author > b.author;
            });
    }
    else if (sort == "date") {
        std::sort(out.begin(), out.end(),
            [this](const Preset& a, const Preset& b) {
                return sortAsc ? a.date < b.date : a.date > b.date;
            });
    }
    else {
        std::sort(out.begin(), out.end(),
            [this](const Preset& a, const Preset& b) {
                return sortAsc ? a.name < b.name : a.name > b.name;
            });
    }

    return out;
}

std::vector<PresetManager::Preset>
PresetManager::getUserPresets()
{

    String bankDir = File(dir).getChildFile("User").getFullPathName();

    std::vector<Preset> out;
    for (const auto& [key, preset] : cache) {
        if (key.startsWith(bankDir)) {
            out.push_back(preset);
        }
    }

    std::sort(out.begin(), out.end(),
        [this](const Preset& a, const Preset& b) {
            return sortAsc ? a.name < b.name
                : a.name > b.name;
        });

    return out;
}

void PresetManager::loadNextPreset(bool loadPrev)
{
    auto presets = getPresets();
    if (presets.empty()) return;

    auto it = std::find_if(presets.begin(), presets.end(),
        [this](const Preset& p) {
            return p.id == selectedPreset.id;
        });

    if (it != presets.end()) {
        size_t index = std::distance(presets.begin(), it);
        index = loadPrev
            ? (index - 1 + presets.size()) % presets.size()
            : (index + 1) % presets.size();
        loadPreset(presets[index]);
    }
    else {
        return loadPreset(presets[0]);
    }
}

void PresetManager::toggleLiked(Preset preset)
{
    if (!cache.count(preset.path))
        return;

    cache[preset.path].liked = !cache[preset.path].liked;
    saveCache();
}

String PresetManager::getUserPresetPath(String name)
{
    name = File::createLegalFileName(name);
    for (auto& [path, preset] : cache) {
        if (File::createLegalFileName(preset.name) == name) {
            if (getPresetBank(preset.path) == "User")
                return preset.path;
        }
    }

    return "";
}

PresetManager::Preset PresetManager::getPreset(String id)
{
    for (auto& [path, preset] : cache) {
        if (preset.id == id) {
            return preset;
        }
    }
    Preset p;
    return p;
}

String PresetManager::getPresetBank(String path)
{
    String relative = File(path).getRelativePathFrom(dir);

    juce::StringArray parts;
    parts.addTokens(relative, "\\/", ""); // split by
    return parts.size() > 1 ? parts[0] : "";
}

void PresetManager::exportBank(std::vector<Preset> presets, String bankname, String path)
{
    ZipFile::Builder zipBuilder;
    std::vector<std::unique_ptr<MemoryBlock>> dataBlocks;
    std::vector<File> tempFiles;

    for (auto& preset : presets) {
        auto file = File(preset.path);
        if (!file.existsAsFile())
            continue;

        std::unique_ptr<XmlElement> xml = XmlDocument::parse(file);
        if (!xml)
            continue;

        xml->setAttribute("id", Uuid().toString()); // change file Uuid to avoid conflicts
        String xmlString = xml->toString();

        // use a temporary file, I couldn't get memory streams to work
        File temp = File::getSpecialLocation(File::tempDirectory)
            .getChildFile("preset_" + File::createLegalFileName(preset.name) + String::toHexString(Random::getSystemRandom().nextInt()) + ".xml");
        temp.deleteFile();
        temp.replaceWithText(xmlString);
        tempFiles.push_back(temp);

        zipBuilder.addFile(temp, 9, bankname + "/" + file.getFileName());
    }

    File file = File(path);
    FileOutputStream out(file);
    if (out.openedOk()) {
        zipBuilder.writeToStream(out, nullptr);
    }

    for (auto& tf : tempFiles)
        tf.deleteFile();
}

void PresetManager::importBank(File file)
{
    if (!file.existsAsFile())
        return;

    ZipFile zip(file);
    zip.uncompressTo(File(dir), true);
}

PresetManager::FileTree::Folder PresetManager::buildFileTree(const juce::File& directory,
    const juce::String& extension,
    int& nextId)
{
    FileTree::Folder folder;
    folder.name = directory.getFileName();

    // Child folders

    juce::Array<juce::File> childFolders;

    directory.findChildFiles(childFolders,
        juce::File::findDirectories,
        false);

    childFolders.sort();

    for (const auto& childDir : childFolders)
    {
        auto childTree = buildFileTree(childDir,
            extension,
            nextId);

        if (!childTree.children.empty()
            || !childTree.files.empty())
        {
            folder.children.push_back(std::move(childTree));
        }
    }

    // files

    juce::Array<juce::File> files;

    directory.findChildFiles(files,
        juce::File::findFiles,
        false,
        "*" + extension);

    files.sort();

    for (const auto& file : files)
    {
        FileTree::FileEntry entry;

        entry.id = nextId++;
        entry.name = file.getFileNameWithoutExtension();
        entry.file = file;

        folder.files.push_back(std::move(entry));
    }

    return folder;
}

void PresetManager::flattenTree(const FileTree::Folder& folder,
    std::vector<FileTree::FileEntry>& out)
{
    // Add files in this folder
    for (const auto& file : folder.files)
        out.push_back(file);

    // Recurse into children
    for (const auto& child : folder.children)
        flattenTree(child, out);
}