#pragma once
#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class TetraOPAudioProcessor;

class TablesManager
{
public:
	enum WTMode
    {
        Table,
		BasicShapes,
        WhiteNoise,
        PinkNoise,
        UserTable
    };

    struct WTable
    {
        int fileId;
        String name = "Basic Shapes";
		String path;
        float srate;
        gin::Wavetable tables;
        int numTables;
        int tableSize;
        String b64; // raw base 64 data
        WTMode mode = BasicShapes;
    };

    struct TableFile
    {
        int id;
        String name;
        String parent;
        String path;
    };

    struct TableFolder
    {
        String name;
        std::vector<TablesManager::TableFile>files;
        std::vector<TableFolder>children;
    };

    std::atomic<bool> UIDirty[4];
	String dir;
	std::array<WTable, MAX_OSCILLATORS> wavetables{};

    std::vector<TableFile> tableList;

	TablesManager(TetraOPAudioProcessor &p, String tablesFolder);

    void scanTables();
	void reloadWavetables();
	void load(int oscId, WTMode mode, String path);
	void loadBasicShapes(int oscId);
    void loadNoise(int oscId, bool pink);
    void loadFromId(int oscId, int id);
    gin::Wavetable loadWaveTable(float sr, const juce::MemoryBlock& wav, const juce::String& format, int size) const;
    std::vector<TableFolder> getFolderTree();
    ValueTree serialize();
    void unserialize(const ValueTree state);

private:
	TetraOPAudioProcessor& audioProcessor;
};