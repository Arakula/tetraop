#include "../PluginProcessor.h"
#include "TablesManager.h"

TablesManager::TablesManager(TetraOPAudioProcessor& p, String tablesFolder)
	: audioProcessor(p)
	, dir(tablesFolder)
{
	if (!File(dir).exists())
    	File(dir).createDirectory();

    scanTables();
}

static void buildTableList(const juce::File& root, std::vector<TablesManager::TableFile>& list)
{
    std::queue<juce::File> dirs;
    dirs.push(root);

    while (!dirs.empty())
    {
        File dir_ = dirs.front();
        dirs.pop();

        for (const auto& entry : juce::RangedDirectoryIterator(dir_, true, "*", juce::File::findFilesAndDirectories))
        {
            auto file = entry.getFile();

            if (file.isDirectory())
            {
                dirs.push(file);
            }
            else if (file.hasFileExtension("wav;flac"))
            {
                TablesManager::TableFile tf;

                tf.id = (int)list.size();
                tf.name = file.getFileNameWithoutExtension();
                tf.parent = file.getParentDirectory().getRelativePathFrom(root.getParentDirectory());
                tf.path = file.getRelativePathFrom(root.getParentDirectory());
                list.push_back(std::move(tf));
            }
        }
    }
}

void TablesManager::scanTables()
{
    MessageManager::callAsync([this]
        {
            tableList.clear();
            
            for (const auto& entry : juce::RangedDirectoryIterator(dir, false, "*", File::findDirectories))
            {
                buildTableList(entry.getFile(), tableList);
            }
        });
}

/**
 * Reloads wavetables of all oscillators
 */
void TablesManager::reloadWavetables()
{
    for (int i = 0; i < MAX_OSCILLATORS; ++i)
    {
        auto& table = wavetables[i];
        if (table.srate != audioProcessor.osrate || table.numTables == 0)
        {
            load(i, table.mode, table.path);
        }
    }
}

void TablesManager::loadFromId(int oscId, int id)
{
    if (id == -3 || id >= (int)tableList.size())
    {
        load(oscId, WTMode::BasicShapes, "");
    }
    else if (id == -2)
    {
        load(oscId, WTMode::WhiteNoise, "");
    }
    else if (id == -1)
    {
        load(oscId, WTMode::PinkNoise, "");
    }
    else {
        auto& t = tableList[id];
        load(oscId, WTMode::Table, t.path);
    }
}

void TablesManager::load(int oscId, WTMode mode, String path)
{
	WTable& table = wavetables[oscId];
	MemoryBlock block;
	String format;

	if (mode == BasicShapes)
	{
        loadBasicShapes(oscId);
        return;
	}
	else if (mode == WhiteNoise)
	{
        loadNoise(oscId, false);
		return;
	}
	else if (mode == PinkNoise)
	{
        loadNoise(oscId, true);
		return;
	}
    else if (mode == UserTable)
    {
        loadUserTable(oscId, path, table.b64);
        return;
    }

    auto file = File(dir).getChildFile(path);
    format = file.getFileExtension().substring(1);
    if (!file.existsAsFile() || (format != "wav" && format != "flac"))
    {
        loadBasicShapes(oscId);
        return;
    }

    file.loadFileAsData(block);
    table.fileId = -10;
    for (int i = 0; i < tableList.size(); ++i)
    {
        if (tableList[i].path == path)
        {
            table.fileId = i;
            break;
        }
    }

    table.name = file.getFileNameWithoutExtension();

	auto data = loadWaveTable(audioProcessor.osrate, block, format, 0);
    auto numTables = data.getNumTables();
    if (numTables == 0)
    {
        loadBasicShapes(oscId);
        return;
    }

    auto tablesz = data.getUnchecked(0)->tableSize;
	if (tablesz == 0)
	{
        loadBasicShapes(oscId);
        return;
	}

	juce::ScopedLock sl(audioProcessor.dspLock);
    table.mode = mode;
    table.numTables = numTables;
    table.tableSize = tablesz;
    table.path = path;
    table.srate = audioProcessor.osrate;
	std::swap(table.tables, data);
    UIDirty[oscId].store(true);
}

void TablesManager::loadUserTable(int oscId, String path, String b64)
{
    WTable& table = wavetables[oscId];
    MemoryBlock block;
    String format;

    auto file = File(dir).getChildFile(path);
    format = file.getFileExtension().substring(1);
    table.fileId = -10;
    table.name = file.getFileNameWithoutExtension();

    juce::MemoryOutputStream stream(block, false);
    if (!juce::Base64::convertFromBase64(stream, b64))
    {
        loadBasicShapes(oscId);
        return;
    }

    auto data = loadWaveTable(audioProcessor.osrate, block, format, 0);
    auto numTables = data.getNumTables();
    if (numTables == 0)
    {
        loadBasicShapes(oscId);
        return;
    }

    auto tablesz = data.getUnchecked(0)->tableSize;
    if (tablesz == 0)
    {
        loadBasicShapes(oscId);
        return;
    }

    table.b64 = b64;
    juce::ScopedLock sl(audioProcessor.dspLock);
    table.mode = UserTable;
    table.numTables = numTables;
    table.tableSize = tablesz;
    table.path = path;
    table.srate = audioProcessor.osrate;
    std::swap(table.tables, data);
    UIDirty[oscId].store(true);
}

void TablesManager::loadNext(int oscId)
{
    auto& table = wavetables[oscId];
    int id = table.fileId + 1;

    if (id < -2 || id >= (int)tableList.size())
        return loadBasicShapes(oscId);

    if (id == -2)
        return loadNoise(oscId, false);

    if (id == -1)
        return loadNoise(oscId, true);

    if (tableList.empty())
        return loadBasicShapes(oscId);

    auto& file = tableList[id % tableList.size()];
    load(oscId, Table, file.path);
}

void TablesManager::loadPrev(int oscId)
{
    auto& table = wavetables[oscId];
    int id = table.fileId - 1;
    
    if (id < -3)
    {
        if (tableList.empty())
            return loadNoise(oscId, true);

        auto& file = tableList.back();
        return load(oscId, Table, file.path);
    }

    if (id == -3)
        return loadBasicShapes(oscId);

    if (id == -2)
        return loadNoise(oscId, false);

    if (id == -1)
        return loadNoise(oscId, true);

    if (tableList.empty())
        return loadNoise(oscId, true);

    auto& file = tableList[id % tableList.size()];
    load(oscId, Table, file.path);

}

void TablesManager::loadBasicShapes(int oscId)
{
    WTable& table = wavetables[oscId];
    MemoryBlock block(BinaryData::Basic_Shapes_wt2048, BinaryData::Basic_Shapes_wt2048Size);
    table.fileId = -3;
    table.name = "Basic Shapes";
    table.path = "";
    block = MemoryBlock(BinaryData::Basic_Shapes_wt2048, BinaryData::Basic_Shapes_wt2048Size);
    
    auto data = loadWaveTable(audioProcessor.osrate, block, "flac", 2048);
    auto numTables = data.getNumTables();
    auto tablesz = data.getUnchecked(0)->tableSize;

    juce::ScopedLock sl(audioProcessor.dspLock);
    table.mode = BasicShapes;
    table.numTables = numTables;
    table.tableSize = tablesz;
    table.srate = audioProcessor.osrate;
    std::swap(table.tables, data);
    UIDirty[oscId].store(true);
}

void TablesManager::loadNoise(int oscId, bool pink)
{
    WTable& table = wavetables[oscId];
    table.fileId = pink ? -1 : -2;
    table.name = pink ? "Pink Noise" : "White Noise";
    table.path = "";
    juce::ScopedLock sl(audioProcessor.dspLock);
    table.mode = pink ? PinkNoise : WhiteNoise;
    table.tables = gin::Wavetable();
    table.numTables = 0;
    table.tableSize = 0;
    UIDirty[oscId].store(true);
}


gin::Wavetable TablesManager::loadWaveTable(float sr, const juce::MemoryBlock& wav, const juce::String& format, int size) const
{
    auto is = new juce::MemoryInputStream(wav, false);

    if (format == "wav")
    {
        if (auto reader = std::unique_ptr<juce::AudioFormatReader>(juce::WavAudioFormat().createReaderFor(is, true)))
        {
            if (size <= 0)
                size = gin::getWavetableSize(wav);

            if (size <= 0)
                if (reader->lengthInSamples % 2048 == 0)
                    size = 2048;
                else if (reader->lengthInSamples % 1024 == 0)
                    size = 1024;
                else if (reader->lengthInSamples % 512 == 0)
                    size = 512;
                else if (reader->lengthInSamples % 256 == 0)
                    size = 256;
                else if (reader->lengthInSamples % 128 == 0)
                    size = 128;
                else
                    size = int(reader->lengthInSamples / 2048);

            if (size > 0)
            {
                int samplesToUse = int(reader->lengthInSamples);
                int frames = samplesToUse / size;

                samplesToUse = frames * size;

                juce::AudioSampleBuffer buf(1, samplesToUse);
                reader->read(&buf, 0, samplesToUse, 0, true, false);

                gin::Wavetable t;
                loadWavetables(t, sr, buf, reader->sampleRate, size);

                // pad tables for cubic interpolation
                for (int i = 0; i < t.getNumTables(); ++i)
                {
                    auto& tables = t.getTable(i)->tables;
                    auto ntables = tables.size();
                    for (int j = 0; j < ntables; ++j)
                    {
                        auto& tbl = tables[j];
                        size_t tablesz = tbl.size();
                        float y0 = tbl[tablesz - 1];
                        float y1 = tbl[0];
                        float y2 = tbl[1];
                        tbl.resize(tablesz + 3);
                        memmove(&tbl[1], &tbl[0], tablesz * sizeof(float));
                        tbl[0] = y0;
                        tbl[tablesz + 1] = y1;
                        tbl[tablesz + 2] = y2;
                    }
                }

                return t;
            }
        }
    }
    else if (format == "flac")
    {
        if (auto reader = std::unique_ptr<juce::AudioFormatReader>(juce::FlacAudioFormat().createReaderFor(is, true)))
        {
            juce::AudioSampleBuffer buf(1, int(reader->lengthInSamples));
            reader->read(&buf, 0, int(reader->lengthInSamples), 0, true, false);

            if (size <= 0)
                if (reader->lengthInSamples % 2048 == 0)
                    size = 2048;
                else if (reader->lengthInSamples % 1024 == 0)
                    size = 1024;
                else if (reader->lengthInSamples % 512 == 0)
                    size = 512;
                else if (reader->lengthInSamples % 256 == 0)
                    size = 256;
                else if (reader->lengthInSamples % 128 == 0)
                    size = 128;
                else
                    size = (int)(reader->lengthInSamples / 2048);

            if (size <= 0)
                return {};

            gin::Wavetable t;
            loadWavetables(t, sr, buf, reader->sampleRate, size);


            // pad tables for cubic interpolation
            for (int i = 0; i < t.getNumTables(); ++i)
            {
                auto& tables = t.getTable(i)->tables;
                auto ntables = tables.size();
                for (int j = 0; j < ntables; ++j)
                {
                    auto& tbl = tables[j];
                    size_t tablesz = tbl.size();
                    float y0 = tbl[tablesz - 1];
                    float y1 = tbl[0];
                    float y2 = tbl[1];
                    tbl.resize(tablesz + 3);
                    memmove(&tbl[1], &tbl[0], tablesz * sizeof(float));
                    tbl[0] = y0;
                    tbl[tablesz + 1] = y1;
                    tbl[tablesz + 2] = y2;
                }
            }
            return t;
        }
    }

    return {};
}

static TablesManager::TableFolder* findOrCreateChild(TablesManager::TableFolder& parent, const juce::String& name)
{
    for (auto& child : parent.children)
        if (child.name == name)
            return &child;

    parent.children.push_back({ name });
    return &parent.children.back();
}

std::vector<TablesManager::TableFolder> TablesManager::getFolderTree()
{
    TableFolder root;
    root.name = "";
    String sep = File().getSeparatorString();

    for (const auto& file : tableList)
    {
        TableFolder* current = &root;

        auto parts = juce::StringArray::fromTokens(file.parent, sep, "");

        for (auto& part : parts)
            current = findOrCreateChild(*current, part);

        current->files.push_back(file);
    }

    return root.children;
}

ValueTree TablesManager::serialize()
{
    auto tree = ValueTree("WAVETABLES");

    int i = 0;
    for (const auto& table : wavetables)
    {
        auto element = ValueTree("Table");
        element.setProperty("id", var(i), nullptr);
        element.setProperty("path", var(table.path), nullptr);
        element.setProperty("mode", var(table.mode), nullptr);
        element.setProperty("data", table.mode == UserTable ? var(table.b64) : var(""), nullptr);
        tree.appendChild(element, nullptr);
        i += 1;
    }

    return tree;
}

void TablesManager::unserialize(const ValueTree state)
{
    for (int i = 0; i < state.getNumChildren(); ++i) {
        ValueTree tree = state.getChild(i);

        const auto id = (int)tree.getProperty("id");
        const auto mode = (WTMode)(int)tree.getProperty("mode");
        const auto path = tree.getProperty("path").toString();
        const auto data = tree.getProperty("data").toString();

        auto& table = wavetables[id];
        table.b64 = data;

        load(id, mode, path);
    }
}