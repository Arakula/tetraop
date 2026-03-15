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

void TablesManager::load(int oscId, WTMode mode, String path)
{
	WTable& table = wavetables[oscId];
	MemoryBlock block;
	String format;
	int size = 0;

	table.mode = mode;

	if (mode == BasicShapes)
	{
        table.fileId = -3;
		table.name = "Basic Shapes";
		block = MemoryBlock(BinaryData::Basic_Shapes_wt2048, BinaryData::Basic_Shapes_wt2048Size);
		format = "flac";
		size = 2048;
	}
	else if (mode == WhiteNoise)
	{
        table.fileId = -2;
		table.name = "White Noise";
		return;
	}
	else if (mode == PinkNoise)
	{
        table.fileId = -1;
		table.name = "Pink Noise";
		return;
	}
	else if (mode == Table)
	{
		File file = File(path);
		table.name = file.getFileNameWithoutExtension();
		format = file.getFileExtension().toLowerCase();
		if (format != "wav" || format != "flac")
		{
			return load(oscId, BasicShapes, "");
		}

        // TODO attempt to find from relative path using dir and user dirs
        // if found load block
        // else load basic shapes
	}

	auto data = loadWaveTable(audioProcessor.osrate, block, format, size);
	table.numTables = data.getNumTables();
	table.tableSize = data.getUnchecked(0)->tableSize;

	if (table.numTables == 0 || table.tableSize == 0)
	{
		return load(oscId, BasicShapes, "");
	}

	juce::ScopedLock sl(audioProcessor.dspLock);
	std::swap(table.tables, data);
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