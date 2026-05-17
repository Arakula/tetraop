#include "Header.h"
#include "../../PluginEditor.h"


void CPUMeter::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	g.setColour(COLOR_PANEL());
	g.fillRoundedRectangle(b, 3.f);

    g.setFont(FontOptions(16.f));
    g.setColour(COLOR_KNOB_LABEL());

	auto pad = 0.f;
	auto w = (b.getWidth() - pad * 2);
	auto x = pad + w / 4 / 2;

	UIUtils::drawCPU(g, Rectangle<float>(x - 6, 6.f, 16.f, 16.f), COLOR_KNOB_LABEL());
	x += w / 4;

    g.drawText(String((int)editor.audioProcessor.synth->getCpuUsage()), Rectangle<float>(x - 16.f, 6.f, 32.f, 16.f), Justification::centred);
	x += w / 4;

	UIUtils::drawVoices(g, Rectangle<float>(x - 6, 6.f, 16.f, 16.f), COLOR_KNOB_LABEL());
	x += w / 4;

    g.drawText(String(editor.audioProcessor.synth->getNumActiveVoices()), Rectangle<float>(x - 16.f, 6.f, 32.f, 16.f), Justification::centred);

}

///////////////////////////////////////////////

Header::Header(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
	startTimerHz(5);

	addAndMakeVisible(logo);
	logo.setAlpha(0.f);
	logo.onClick = [this] { editor.showAboutDialog();};

	addAndMakeVisible(lib);
	lib.setAlpha(0.f);
	lib.setVisible(false);
	lib.onClick = [this]
		{
			editor.selectTab(4);
			toggleUIComponents();
		};

	addAndMakeVisible(syn);
	syn.setAlpha(0.f);
	syn.onClick = [this]
		{
			editor.selectTab(0);
			toggleUIComponents();
		};

	addAndMakeVisible(fxs);
	fxs.setAlpha(0.f);
	fxs.onClick = [this]
		{
			editor.selectTab(1);
			toggleUIComponents();
		};

	addAndMakeVisible(mod);
	mod.setAlpha(0.f);
	mod.onClick = [this]
		{
			editor.selectTab(2);
			toggleUIComponents();
		};

	addAndMakeVisible(cfg);
	cfg.setAlpha(0.f);
	cfg.onClick = [this]
		{
			editor.selectTab(3);
			toggleUIComponents();
		};

	addAndMakeVisible(prevPreset);
	prevPreset.setAlpha(0.f);
	prevPreset.onClick = [this] { selectNextPreset(true); };

	addAndMakeVisible(nextPreset);
	nextPreset.setAlpha(0.f);
	nextPreset.onClick = [this] { selectNextPreset(false); };

	addAndMakeVisible(preset);
	preset.setAlpha(0.f);
	preset.onClick = [this] { showPresets(); };

	addAndMakeVisible(saveBtn);
	saveBtn.setAlpha(0.f);
	saveBtn.onClick = [this] { savePreset(); };

	addAndMakeVisible(undoBtn);
	undoBtn.setAlpha(0.f);
	undoBtn.onClick = [this] { editor.audioProcessor.undomgr->undo(); };

	addAndMakeVisible(redoBtn);
	redoBtn.setAlpha(0.f);
	redoBtn.onClick = [this] { editor.audioProcessor.undomgr->redo(); };

	cpuMeter = std::make_unique<CPUMeter>(editor);
	addAndMakeVisible(cpuMeter.get());

	gain = std::make_unique<Rotary>(editor, "master_gain", "", Rotary::gain2dB);
	addAndMakeVisible(gain.get());
	gain->labelSize = 11.f;
	gain->setSmall();
	gain->setDark();
	gain->yoffset += 4;
	editor.registerModParam(gain.get(), TetraOPAudioProcessorEditor::kGlobal);

	meter = std::make_unique<Meter>(editor.audioProcessor);
	addAndMakeVisible(meter.get());
}

Header::~Header()
{
}

void Header::timerCallback()
{
	if (editor.audioProcessor.presetmgr->nameDirty.exchange(false))
	{
		repaint();
	}
	if (editor.audioProcessor.undomgr->UIDirty.exchange(false))
	{
		repaint();
	}
}

void Header::parameterChanged(const juce::String&, float)
{
	MessageManager::callAsync([this]{ toggleUIComponents(); });
}

void Header::paint(Graphics& g)
{
	g.setColour(COLOR_PANEL().darker(0.6f));
    g.fillRect(getLocalBounds());

	UIUtils::drawLogo(g, logo.getBounds().reduced(5).toFloat(), COLOR_KNOB_LABEL());

	g.setFont(16.f);
	//g.drawText("LIB", lib.getBounds().toFloat(), Justification::centred);
	g.drawText("SYN", syn.getBounds().toFloat(), Justification::centred);
	g.drawText("FXS", fxs.getBounds().toFloat(), Justification::centred);
	g.drawText("MOD", mod.getBounds().toFloat(), Justification::centred);
	g.drawText("CFG", cfg.getBounds().toFloat(), Justification::centred);

	int tab = editor.audioProcessor.selectedTab;
	auto selbounds = (tab == 0 ? syn.getBounds()
		: tab == 1 ? fxs.getBounds()
		: tab == 2 ? mod.getBounds()
		: tab == 3 ? cfg.getBounds()
		: lib.getBounds()).toFloat();

	g.setColour(COLOR_BACKGROUND());
	g.fillRect(selbounds);
	g.fillRoundedRectangle(prevPreset.getBounds().withRight(saveBtn.getRight()).toFloat(), 5.f);

	auto txt = tab == 0 ? "SYN"
		: tab == 1 ? "FXS"
		: tab == 2 ? "MOD"
		: tab == 3 ? "CFG"
		: "LIB";

	g.setColour(COLOR_ACTIVE());
	g.drawText(txt, selbounds, Justification::centred);

	g.setColour(COLOR_KNOB_LABEL());
	g.drawFittedText(editor.audioProcessor.presetmgr->selectedPreset.name, preset.getBounds(), Justification::centred, 1);

	UIUtils::drawTriangle(g, prevPreset.getBounds().toFloat().reduced(10.f), 3, COLOR_KNOB_LABEL());
	UIUtils::drawTriangle(g, nextPreset.getBounds().toFloat().reduced(10.f), 1, COLOR_KNOB_LABEL());
	UIUtils::drawSave(g, saveBtn.getBounds().reduced(5).toFloat(), COLOR_KNOB_LABEL());

	bool canUndo = editor.audioProcessor.undomgr->canUndo();
	bool canRedo = editor.audioProcessor.undomgr->canRedo();
	UIUtils::drawUndo(g, undoBtn.getBounds().reduced(6,0).toFloat(), true, COLOR_KNOB_LABEL().withAlpha(canUndo ? 0.75f : 0.4f));
	UIUtils::drawUndo(g, redoBtn.getBounds().reduced(6,0).toFloat(), false, COLOR_KNOB_LABEL().withAlpha(canRedo ? 0.75f : 0.4f));
}

void Header::resized()
{
	logo.setBounds(0, 0, HEADER_HEIGHT, HEADER_HEIGHT);
	lib.setBounds(70, 0, 56, HEADER_HEIGHT);
	syn.setBounds(lib.getBounds()); // syn button is above lib making it hidden
	fxs.setBounds(syn.getBounds().translated(syn.getWidth(), 0));
	mod.setBounds(fxs.getBounds().translated(fxs.getWidth(), 0));
	cfg.setBounds(mod.getBounds().translated(mod.getWidth(), 0));

	prevPreset.setBounds(325, 8, 28, 28);
	nextPreset.setBounds(prevPreset.getRight(), 8, 28, 28);
	preset.setBounds(nextPreset.getRight(), 8, 228, 28);
	saveBtn.setBounds(preset.getRight(), 8, 28, 28);

	undoBtn.setBounds(saveBtn.getBounds().getRight() + 12, 8, 28, 28);
	redoBtn.setBounds(undoBtn.getBounds().translated(undoBtn.getWidth(), 0));

	cpuMeter->setBounds(redoBtn.getRight() + 12, 8, 90, 28);
	gain->setBounds(Rectangle<int>(HEADER_HEIGHT, HEADER_HEIGHT).withY(0).withX(cpuMeter->getRight() + 12).translated(0,5));
	meter->setBounds(Rectangle<int>(28,20).withY(14).withX(gain->getRight() + 12).withRight(getRight() - 12));
}


void Header::toggleUIComponents()
{
	MessageManager::callAsync([this] { repaint(); });
}

static const PresetManager::FileTree::FileEntry* findFileById(
	const std::vector<PresetManager::FileTree::Folder>& folders,
	int id)
{
	for (const auto& folder : folders)
	{
		for (const auto& file : folder.files)
		{
			if (file.id == id)
				return &file;
		}

		if (auto* result = findFileById(folder.children, id))
			return result;
	}

	return nullptr;
}

static const PresetManager::FileTree::FileEntry* findFileByName(
	const std::vector<PresetManager::FileTree::Folder>& folders,
	String name)
{
	for (const auto& folder : folders)
	{
		for (const auto& file : folder.files)
		{
			if (file.name == name)
				return &file;
		}

		if (auto* result = findFileByName(folder.children, name))
			return result;
	}

	return nullptr;
}

static void buildPresetsMenu(juce::PopupMenu& menu,
	const std::vector<PresetManager::FileTree::Folder>& folders,
	String selected)
{
	for (const auto& folder : folders)
	{
		juce::PopupMenu sub;

		if (!folder.children.empty())
		{
			buildPresetsMenu(sub,
				folder.children,
				selected);
		}

		for (const auto& file : folder.files)
		{
			if (sub.getNumItems() > 0 && sub.getNumItems() % 20 == 0)
				sub.addColumnBreak();

			sub.addItem(file.id,
				file.name,
				true,
				selected == file.name);
		}

		if (sub.getNumItems() > 0)
			menu.addSubMenu(folder.name, sub);
	}
}

void Header::showPresets()
{
	String pname = editor.audioProcessor.presetmgr->selectedPreset.name;
	String dir = editor.audioProcessor.presetmgr->dir;

	PopupMenu menu;
	menu.addItem(1, "-- Init --", true, pname == "-- Init --");
	menu.addSeparator();

	int startId = 2;
	auto fileTree = editor.audioProcessor.presetmgr->buildFileTree(dir, ".xml", startId);
	buildPresetsMenu(menu, fileTree.children, pname);

	menu.addSeparator();
	menu.addItem(99999, "Open Folder");

	auto menuPos = localPointToGlobal(preset.getBounds().getBottomLeft().translated(20,0));
	menu.showMenuAsync(PopupMenu::Options()
		.withTargetComponent(*this)
		.withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this, fileTree](int result) {
			if (result == 0) return;

			if (result == 1)
			{
				editor.audioProcessor.undomgr->createUndo();
				editor.audioProcessor.presetmgr->loadInit();
				return;
			}

			if (result == 99999)
			{
				auto dir = File(editor.audioProcessor.presetmgr->dir);
				dir.startAsProcess();
				return;
			}

			if (auto* file = findFileById(fileTree.children, result))
			{
				editor.audioProcessor.undomgr->createUndo();
				editor.audioProcessor.presetmgr->loadPresetFromPath(file->file.getFullPathName());
				return;
			}
		});
}

void Header::savePreset()
{
	int startId = 0;
	auto& mgr = editor.audioProcessor.presetmgr;
	auto fileTree = mgr->buildFileTree(mgr->dir, ".xml", startId);
	auto selPreset = findFileByName(fileTree.children, mgr->selectedPreset.name);

	String dir = selPreset
		? selPreset->file.getFullPathName()
		: mgr->userDir.getFullPathName();

	fileChooser.reset(new juce::FileChooser("Save preset", dir, "*.xml"));
	fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
		[this](const juce::FileChooser& fc)
		{
			auto file = fc.getResult();
			if (file.isDirectory()) return;
			if (file == juce::File()) return;

			auto& mgr = editor.audioProcessor.presetmgr;
			auto preset = mgr->getPreset(mgr->selectedPreset.id);
			preset.name = file.getFileNameWithoutExtension();
			auto filestr = mgr->exportPreset(preset);

			file.replaceWithText(filestr);
			editor.audioProcessor.presetmgr->loadPresetFromPath(file.getFullPathName());
		});
}

void Header::selectNextPreset(bool isNext)
{
	int startId = 0;
	auto& mgr = editor.audioProcessor.presetmgr;
	auto tree = mgr->buildFileTree(
		mgr->dir, ".xml", startId
	);

	String fname = mgr->selectedPreset.name;

	std::vector<PresetManager::FileTree::FileEntry> presets;
	mgr->flattenTree(tree, presets);

	if (presets.empty())
		return;

	if (fname == "-- Init --")
	{
		if (presets.size() > 0)
			mgr->loadPresetFromPath(isNext 
				? presets[0].file.getFullPathName()
				: presets.back().file.getFullPathName()
			);
		return;
	}

	for (int i = 0; i < presets.size(); ++i)
	{
		if (presets[i].file.getFileNameWithoutExtension() == fname)
		{
			int index = isNext ? i + 1 : i - 1;
			if (index == presets.size() || index < 0)
				mgr->loadInit();
			else 
				mgr->loadPresetFromPath(presets[index].file.getFullPathName());
			return;
		}
	}

	// preset not found, revert to init
	mgr->loadInit();
}