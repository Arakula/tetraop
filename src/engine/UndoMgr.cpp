#include "UndoMgr.h"
#include "../PluginProcessor.h"

UndoMgr::UndoMgr(TetraOPAudioProcessor& audioProcessor)
	: audioProcessor(audioProcessor)
{
}

UndoMgr::Snapshot UndoMgr::createSnapshot()
{
	juce::MemoryBlock state;
    audioProcessor.getStateInformation(state);
    std::unique_ptr<juce::XmlElement> xml(
        audioProcessor.getXmlFromBinary(state.getData(), (int)state.getSize())
    );

	Snapshot snap;
	snap.xml = xml->toString();
	PresetManager::Preset selectedPreset = audioProcessor.presetmgr->selectedPreset;
	snap.hash = (selectedPreset.id
		+ selectedPreset.name
		+ snap.xml).hash();

	return snap;
}

void UndoMgr::applySnapshot(Snapshot snap)
{
	std::unique_ptr<XmlElement> xml = XmlDocument::parse(snap.xml);
	if (!xml)return;
	isUndoing = true;
	juce::MemoryBlock state;
	audioProcessor.copyXmlToBinary(*xml, state);
	audioProcessor.setStateInformation(state.getData(), (int)state.getSize());
	isUndoing = false;
}

void UndoMgr::createUndo()
{
	if (undoStack.size() > globals::MAX_UNDO) {
		undoStack.erase(undoStack.begin());
	}

    Snapshot snap = createSnapshot();

	if (undoStack.empty() || undoStack.back().hash != snap.hash) {
		undoStack.push_back(snap);
	}

	redoStack.clear();
	UIDirty.store(true);
}

void UndoMgr::undo()
{
	if (!canUndo())
        return;

    redoStack.push_back(createSnapshot());
    auto& snap = undoStack.back();

	applySnapshot(snap);

    undoStack.pop_back();
	UIDirty.store(true);
}

void UndoMgr::redo()
{
 	if (!canRedo())
        return;

	undoStack.push_back(createSnapshot());
    auto& snap = redoStack.back();

	applySnapshot(snap);

	redoStack.pop_back();
	UIDirty.store(true);
}

void UndoMgr::clear()
{
	undoStack.clear();
    redoStack.clear();
	UIDirty.store(true);
}

bool UndoMgr::canUndo()
{
	return undoStack.size() > 1; // first snapshot is ignored
}

bool UndoMgr::canRedo()
{
	return (bool)redoStack.size();
}
