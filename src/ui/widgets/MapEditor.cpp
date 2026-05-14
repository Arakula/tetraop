#include "MapEditor.h"
#include "../../PluginEditor.h"

MapEditor::MapEditor(TetraOPAudioProcessorEditor& e, std::function<void()>_onClose)
	: onClose(_onClose)
	, editor(e)
{
	startTimerHz(60);
	addAndMakeVisible(enableBtn);
	enableBtn.setAlpha(0.f);
	enableBtn.onClick = [this]
		{
			editor.audioProcessor.undomgr->createUndo();
			editor.audioProcessor.modulation->setConnectionMapped(conn.src, conn.dst, !conn.mapped);
		};

	addAndMakeVisible(gridXBtn);
	gridXBtn.setAlpha(0.f);
	gridXBtn.onClick = [this] { showGridMenu(false); };

	addAndMakeVisible(gridYBtn);
	gridYBtn.setAlpha(0.f);
	gridYBtn.onClick = [this] { showGridMenu(true); };

	addAndMakeVisible(rotLBtn);
	rotLBtn.setAlpha(0.f);
	rotLBtn.onClick = [this]
		{
			editor.audioProcessor.undomgr->createUndo();
			pat.rotate(-1.f / display->gridX);
			pat.buildSegments();
			updateConnectionPointsFromPat();
		};

	addAndMakeVisible(rotRBtn);
	rotRBtn.setAlpha(0.f);
	rotRBtn.onClick = [this]
		{
			editor.audioProcessor.undomgr->createUndo();
			pat.rotate(1.f / display->gridX);
			pat.buildSegments();
			updateConnectionPointsFromPat();
		};

	addAndMakeVisible(sineBtn);
	sineBtn.setAlpha(0.f);
	sineBtn.onClick = [this]
		{
			editor.audioProcessor.undomgr->createUndo();
			bool isRoundCurve = pat.points.size() && pat.points[0].type == Pattern::SinCurve;
			for (auto& point : pat.points)
				point.type = isRoundCurve ? Pattern::Curve : Pattern::SinCurve;
			pat.buildSegments();
			updateConnectionPointsFromPat();
			repaint();
		};

	addAndMakeVisible(closeBtn);
	closeBtn.setAlpha(0.f);
	closeBtn.onClick = [this]
		{
			onClose();
		};

	addAndMakeVisible(fileBtn);
	fileBtn.setAlpha(0.f);
	fileBtn.onClick = [this] { showFileMenu(); };

	display = std::make_unique<CurveEditor>(e, &pat, 10.f, COLOR_ACTIVE(), COLOR_LFO(), false, true);
	display->point_radius = 2.5f;
	display->mpoint_radius = 2.5f;
	display->point_hover = 6.f;
	display->gridY = gridY;
	display->gridX = gridX;
	display->onChange = [this]
		{
			updateConnectionPointsFromPat();
		};
	display->onWheel = [this](const juce::MouseEvent& me, const juce::MouseWheelDetails& wheel)
		{
			auto step = UIUtils::wheelStep(wheel, wheelAccum);
			if (step == 0) return;
			int dir = step > 0 ? 1 : -1;
			for (int n = std::abs(step); n > 0; --n) {
				if (me.mods.isShiftDown()) {
					gridY = gridY + dir;
					// 2...16,32,64,128
					if (gridY < 2) gridY = 128;
					if (gridY == 127) gridY = 64;
					if (gridY == 63) gridY = 32;
					if (gridY == 31) gridY = 16;
					if (gridY == 17) gridY = 32;
					if (gridY == 33) gridY = 64;
					if (gridY == 65) gridY = 128;
					if (gridY == 129) gridY = 2;
				} else {
					gridX = gridX + dir;
					// 2...16,32,64,128
					if (gridX < 2) gridX = 128;
					if (gridX == 127) gridX = 64;
					if (gridX == 63) gridX = 32;
					if (gridX == 31) gridX = 16;
					if (gridX == 17) gridX = 32;
					if (gridX == 33) gridX = 64;
					if (gridX == 65) gridX = 128;
					if (gridX == 129) gridX = 2;
				}
			}
			display->gridY = gridY;
			display->gridX = gridX;
			repaint();
		};
	Modulation::stringToMap(conn.mpoints.toStdString(), pat);
	addAndMakeVisible(display.get());
}

MapEditor::~MapEditor()
{
}

void MapEditor::updateConnectionPointsFromPat()
{
	auto mpoints = Modulation::mapToString(pat);
	conn.mpoints = mpoints;
	editor.audioProcessor.modulation->setConnectionMPoints(conn.src, conn.dst, mpoints);
}

void MapEditor::timerCallback()
{
	if (!isVisible()) return;
	auto& mod = editor.audioProcessor.modulation->modulators[conn.src];
	auto src = juce::String(conn.src);

	bool drawSeek = mod.active && mod.connections;
	float seekPos = mod.value;

	if (display->drawBasicSeek != drawSeek || display->seekPos != seekPos) {
		display->drawBasicSeek = drawSeek;
		display->seekPos = seekPos;
		display->repaint();
	}
}

void MapEditor::setConnection(Modulation::Connection _conn)
{
	if (conn.src == _conn.src && conn.dst == _conn.dst && conn.id == _conn.id &&
		conn.mpoints == _conn.mpoints && conn.mapped == _conn.mapped
	) {
		return;
	}

	if (_conn.src == "key" && conn.src != "key") {
		gridBackup = gridX;
		gridX = 128;
		display->gridX = gridX;
	}
	if (_conn.src != "key" && conn.src == "key") {
		gridX = gridBackup;
		display->gridX = gridX;
	}

	bool patchanged = _conn.mpoints != conn.mpoints;
	conn = _conn;
	if (patchanged) {
		display->clearSelection();
		Modulation::stringToMap(conn.mpoints, pat);
	}
	repaint();
}

void MapEditor::paint(juce::Graphics& g)
{
	UIUtils::drawBevel(g, getLocalBounds().toFloat().reduced(0.5f), PANEL_CORNER, COLOR_BEVEL());
	UIUtils::drawPowerButton(g, enableBtn.getBounds().withWidth(25).toFloat(), conn.mapped ? COLOR_ACTIVE() : COLOR_KNOB_LABEL().withAlpha(0.5f), 1.f);
	g.setColour(conn.mapped ? COLOR_ACTIVE() : COLOR_KNOB_LABEL().withAlpha(0.5f));
	g.setFont(juce::FontOptions(15.f));
	g.drawText(conn.mapped ? "Enabled" : "Disabled", enableBtn.getBounds().withTrimmedLeft(25).toFloat(), juce::Justification::centredLeft);

	bool isSineWave = pat.points.size() && pat.points[0].type == Pattern::SinCurve;
	g.setColour(COLOR_KNOB_LABEL().withAlpha(0.5f));
	g.drawText("Grid", gridXBtn.getBounds().toFloat(), juce::Justification::centredLeft);
	g.drawText(juce::String(gridX), gridXBtn.getBounds().toFloat(), juce::Justification::centredRight);
	g.drawText(juce::String(gridY), gridYBtn.getBounds().toFloat(), juce::Justification::centredLeft);
	UIUtils::drawTriangle(g, rotLBtn.getBounds().toFloat().reduced(6.f), 3, COLOR_KNOB_LABEL().withAlpha(0.5f));
	UIUtils::drawTriangle(g, rotRBtn.getBounds().toFloat().reduced(6.f), 1, COLOR_KNOB_LABEL().withAlpha(0.5f));
	UIUtils::drawSineWave(g, sineBtn.getBounds().toFloat().reduced(2.f, 3.f), 2, isSineWave ? COLOR_ACTIVE() : COLOR_KNOB_LABEL().withAlpha(0.5f));

	UIUtils::drawClose(g, closeBtn.getBounds().reduced(5).toFloat(), COLOR_KNOB_LABEL().withAlpha(0.5f), 2.f);
	g.setColour(COLOR_KNOB_LABEL().withAlpha(0.5f));
	g.drawText("File", fileBtn.getBounds().toFloat(), juce::Justification::centredLeft);
	UIUtils::drawTriangle(g, fileBtn.getBounds().withWidth(25).withRightX(fileBtn.getRight()).reduced(8).toFloat(), 2, COLOR_KNOB_LABEL().withAlpha(0.5f));

	auto text = UIUtils::aliasModulator(conn.src) + " " + juce::String::charToString(0x2192) + " " + UIUtils::aliasParameter(conn.dst);
	g.drawText(text, sineBtn.getBounds().translated(70, 0).withRight(fileBtn.getX() - 10).toFloat(), juce::Justification::centredLeft);
}

void MapEditor::resized()
{
	auto b = getLocalBounds();
	auto pad = 10;
	enableBtn.setBounds(juce::Rectangle<int>(100, 20).withX(b.getX() + pad).withY(b.getY() + 5));
	gridXBtn.setBounds(enableBtn.getBounds().withWidth(50).translated(enableBtn.getWidth() + pad, 0));
	gridYBtn.setBounds(gridXBtn.getBounds().withWidth(30).translated(gridXBtn.getWidth() + pad, 0));
	rotLBtn.setBounds(gridYBtn.getBounds().withWidth(20).translated(gridYBtn.getWidth() + pad, 0));
	rotRBtn.setBounds(rotLBtn.getBounds().translated(rotLBtn.getWidth(), 0));
	sineBtn.setBounds(rotRBtn.getBounds().withWidth(40).translated(rotRBtn.getWidth() + pad*2, 0));

	closeBtn.setBounds(juce::Rectangle<int>(20, 20).withRightX(b.getRight() - pad).withY(b.getY() + 5));
	fileBtn.setBounds(closeBtn.getBounds().withWidth(50).withRightX(closeBtn.getX() - pad));

	display->setBounds(b.withTrimmedTop(30 - 10));
	repaint();
}

void MapEditor::showGridMenu(bool gridy)
{
	juce::PopupMenu menu;
	menu.addSectionHeader(gridy ? "Grid Y" : "Grid X");
	for (int i = 2; i <= 16; ++i) {
		menu.addItem(i, juce::String(i), true, gridy ? gridY == i : gridX == i);
		if (i == 10) menu.addColumnBreak();
	}
	menu.addItem(32, juce::String(32), true, gridy ? gridY == 32 : gridX == 32);
	menu.addItem(64, juce::String(64), true, gridy ? gridY == 64 : gridX == 64);
	menu.addItem(128, juce::String(128), true, gridy ? gridY == 128 : gridX == 128);

	auto menuPos = localPointToGlobal((gridy ? gridYBtn : gridXBtn).getBounds().getBottomLeft());
	menu.setLookAndFeel(&getLookAndFeel());
	menu.showMenuAsync(juce::PopupMenu::Options()
		.withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
		.withTargetComponent(*this)
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this, gridy](int result) {
			if (result == 0) return;
			if (gridy) gridY = result;
			else gridX = result;
			display->gridX = gridX;
			display->gridY = gridY;
			repaint();
		});
}

void MapEditor::showFileMenu()
{
	juce::PopupMenu menu;
	menu.addItem(1, "Reset");
	menu.addItem(2, "Copy");
	menu.addItem(3, "Paste", !Pattern::copy_pattern.empty());

	auto menuPos = localPointToGlobal(fileBtn.getBounds().getBottomLeft());
	menu.setLookAndFeel(&getLookAndFeel());
	menu.showMenuAsync(juce::PopupMenu::Options()
		.withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
		.withTargetComponent(*this)
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this](int result) {
			if (result == 0) return;

			if (result == 1) {
				editor.audioProcessor.undomgr->createUndo();
				pat.clear();
				pat.insertPoint(0.f, 0.f, 0.f, 1);
				pat.insertPoint(1.f, 1.f, 0.f, 1);
				pat.buildSegments();
				updateConnectionPointsFromPat();
			}

			if (result == 2) {
				pat.copy();
			}

			if (result == 3) {
				editor.audioProcessor.undomgr->createUndo();
				pat.paste();
				pat.buildSegments();
				updateConnectionPointsFromPat();
			}

			repaint();
		});
}
