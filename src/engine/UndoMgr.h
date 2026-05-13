// Copyright (C) 2025 tilr
#pragma once
#include <JuceHeader.h>
#include "PresetManager.h"
#include "../Globals.h"

class TetraOPAudioProcessor;

class UndoMgr
{
public:
	std::atomic<bool> UIDirty = false; // flags the header needs redraw
	bool isUndoing = false;

	struct Snapshot {
		PresetManager::Preset selectedPreset;
		juce::String xml;
		size_t hash;
	};

	UndoMgr(TetraOPAudioProcessor& audioProcessor);
	~UndoMgr() {}
	void createUndo();
	void undo();
	void redo();
	void clear();
	bool canUndo();
	bool canRedo();

private:
	Snapshot createSnapshot();
	void applySnapshot(Snapshot snap);

	Snapshot prevState;
	std::vector<Snapshot> undoStack;
	std::vector<Snapshot> redoStack;
	TetraOPAudioProcessor& audioProcessor;
};
