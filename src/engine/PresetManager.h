// Copyright (C) 2025 tilr
#pragma once
#include <JuceHeader.h>

class PresetScanner;
class TetraOPAudioProcessor;

class PresetManager
{
public:
	std::atomic<bool> UIDirty = false; // flags the browser needs a refresh
	std::atomic<bool> nameDirty = false; // flags the header label needs a refresh
	std::atomic<bool> selectedDirty = false; // flags the UI selected patch changed

	struct FileTree
	{
		struct FileEntry
		{
			int id = 0;
			juce::String name;
			juce::File file;
		};

		struct Folder
		{
			juce::String name;

			std::vector<Folder> children;
			std::vector<FileEntry> files;
		};
	};

	struct Preset {
		String version;
		String id;
		String path;
		String name;
		String description;
    	String category;
		String author;
		String bank;
		juce::int64 date;
		juce::int64 fdate;
		bool liked;
		bool exists;
	};

	struct Bank {
		String id;
		std::set<String> categories;
	};

    String dir;
	Preset selectedPreset;
	String selectedCategory = "";
	String selectedBank = "";
	bool liked = false;
	String sort = "name";
	bool sortAsc = true;
	bool isLoadingPreset = false; // used by Demo and Beta versions to prevent loading state other than presets

	PresetManager(TetraOPAudioProcessor& audioProcessor, String dir);
	~PresetManager();

    void scan();
	void saveCache();
	void loadCache();
    void onScanComplete();
	void savePreset(String name, String description, String category, String author, bool overrideByName = false);
	void loadPreset(Preset preset);
	void loadPresetFromPath(String path);
	void loadNextPreset(bool loadPrev);
	String exportPreset(Preset preset);
	String importPreset(File file);
	void loadInit();
	void removePreset(Preset p);
	void setSelected(Preset preset);
	void setBank(String bank, String category, String sort, bool sortAsc, bool liked);
	void toggleLiked(Preset preset);
	String getUserPresetPath(String name);
	String getPresetBank(String path);
	Preset getPreset(String id);
	std::vector<Bank> getBanks();
	std::vector<Preset> getPresets();
	std::vector<Preset> getUserPresets();
	void exportBank(std::vector<Preset> presets, String bankname, String path);
	void importBank(File file);

	FileTree::Folder buildFileTree(const juce::File& directory, const juce::String& extension, int& nextId);


private:
	TetraOPAudioProcessor& audioProcessor;
	std::unordered_map<String, Preset> cache;
	std::unordered_map<String, std::unique_ptr<XmlElement>> snapshots; // used to save preset modifications when switching patch
    std::unique_ptr<PresetScanner> scanner;
};