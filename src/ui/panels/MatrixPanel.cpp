#include "MatrixPanel.h"
#include "../../PluginEditor.h"
#include <functional>

static void showSourceMenu(
	juce::Component* component
	, juce::Point<int> pos
	, std::function<void(juce::String)> cb
) {
	juce::PopupMenu menu;
	menu.addItem(1, "Remove");
	menu.addSeparator();

	juce::PopupMenu envs;
	envs.addItem(21, "Env1");
	envs.addItem(22, "Env2");
	envs.addItem(23, "Env3");
	envs.addItem(24, "Env4");

	juce::PopupMenu lfos;
	lfos.addItem(31, "LFO1");
	lfos.addItem(32, "LFO2");
	lfos.addItem(33, "LFO3");
	lfos.addItem(34, "LFO4");

	juce::PopupMenu macros;
	macros.addItem(41, "Macro1");
	macros.addItem(42, "Macro2");
	macros.addItem(43, "Macro3");
	macros.addItem(44, "Macro4");

	juce::PopupMenu rnds;
	rnds.addItem(61, "Rand Gen 1");
	rnds.addItem(62, "Rand Gen 2");

	juce::PopupMenu mpe;
	mpe.addItem(51, "X");
	mpe.addItem(52, "Y");
	mpe.addItem(53, "Z");
	mpe.addItem(54, "Lift");

	juce::PopupMenu pedals;
	pedals.addItem(71, "Sustain");
	pedals.addItem(72, "Expression");
	pedals.addItem(73, "Soft");
	pedals.addItem(74, "Volume");

	menu.addSubMenu("Envelope", envs);
	menu.addSubMenu("LFO", lfos);
	menu.addSubMenu("Rand", rnds);
	menu.addSubMenu("Macros", macros);
	menu.addSubMenu("MPE", mpe);
	menu.addSubMenu("Pedals", pedals);
	menu.addItem(6, "Mod Wheel");
	menu.addItem(8, "Pitch Bend");
	menu.addItem(4, "Velocity");
	menu.addItem(5, "Key Track");
	menu.addItem(9, "Aftertouch");
	menu.addItem(7, "Random");

	menu.setLookAndFeel(&component->getLookAndFeel());
	menu.showMenuAsync(juce::PopupMenu::Options()
		.withParentComponent(component->findParentComponentOfClass<juce::AudioProcessorEditor>())
		.withTargetComponent(component)
		.withTargetScreenArea({ pos.getX(), pos.getY(), 1, 1 }),
		[cb](int result) {
			juce::String res = "";
			switch (result) {
				case 1: res = "-"; break;
				case 21: res = "env1"; break;
				case 22: res = "env2"; break;
				case 23: res = "env3"; break;
				case 24: res = "env4"; break;
				case 31: res = "lfo1"; break;
				case 32: res = "lfo2"; break;
				case 33: res = "lfo3"; break;
				case 34: res = "lfo4"; break;
				case 41: res = "macro1"; break;
				case 42: res = "macro2"; break;
				case 43: res = "macro3"; break;
				case 44: res = "macro4"; break;
				case 51: res = "x"; break;
				case 52: res = "y"; break;
				case 53: res = "z"; break;
				case 54: res = "lift"; break;
				case 61: res = "rnd1"; break;
				case 62: res = "rnd2"; break;
				case 71: res = "sus"; break;
				case 72: res = "exp"; break;
				case 73: res = "soft"; break;
				case 74: res = "vol"; break;
				case 4: res = "vel"; break;
				case 5: res = "key"; break;
				case 6: res = "mod"; break;
				case 7: res = "rand"; break;
				case 8: res = "bend"; break;
				case 9: res = "at"; break;
			}
			cb(res);
		});
}

static void showDestinationMenu(
    juce::Component* component,
    TetraOPAudioProcessorEditor& editor,
    juce::Point<int> pos,
    std::function<void(juce::String)> cb
) {
    struct FxInfo
    {
        juce::String layer;
        juce::String fxName;
    };

    using FxTree = std::map<
        juce::String, // layer
        std::map<
        juce::String, // fx name
        std::vector<juce::String>
        >
    >;

    // get editor mod params but filter env, lfo and rnd params as these are not always registered
    std::map<juce::String, TetraOPAudioProcessorEditor::ModParam> filteredParams = editor.modulatedParams;
    for (auto it = filteredParams.begin(); it != filteredParams.end(); ) {
        const auto& paramId = it->first;

        if (paramId.startsWith("env") ||
            paramId.startsWith("lfo") ||
            paramId.startsWith("rnd"))
        {
            it = filteredParams.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // add env, lfo and rnd params
    for (int i = 0; i < globals::MAX_ENVELOPES; ++i) {
        filteredParams["env" + juce::String(i + 1) + "_del"] = { nullptr, TetraOPAudioProcessorEditor::kEnvelope };
        filteredParams["env" + juce::String(i + 1) + "_att"] = { nullptr, TetraOPAudioProcessorEditor::kEnvelope };
        filteredParams["env" + juce::String(i + 1) + "_hld"] = { nullptr, TetraOPAudioProcessorEditor::kEnvelope };
        filteredParams["env" + juce::String(i + 1) + "_dec"] = { nullptr, TetraOPAudioProcessorEditor::kEnvelope };
        filteredParams["env" + juce::String(i + 1) + "_sus"] = { nullptr, TetraOPAudioProcessorEditor::kEnvelope };
        filteredParams["env" + juce::String(i + 1) + "_rel"] = { nullptr, TetraOPAudioProcessorEditor::kEnvelope };
        filteredParams["env" + juce::String(i + 1) + "_tenatt"] = { nullptr, TetraOPAudioProcessorEditor::kEnvelope };
        filteredParams["env" + juce::String(i + 1) + "_tendec"] = { nullptr, TetraOPAudioProcessorEditor::kEnvelope };
        filteredParams["env" + juce::String(i + 1) + "_tenrel"] = { nullptr, TetraOPAudioProcessorEditor::kEnvelope };
    }

    for (int i = 0; i < globals::MAX_LFOS; ++i) {
        filteredParams["lfo" + juce::String(i + 1) + "_rate"] = { nullptr, TetraOPAudioProcessorEditor::kLFO };
        filteredParams["lfo" + juce::String(i + 1) + "_rate_sync"] = { nullptr, TetraOPAudioProcessorEditor::kLFO };
        filteredParams["lfo" + juce::String(i + 1) + "_smooth"] = { nullptr, TetraOPAudioProcessorEditor::kLFO };
        filteredParams["lfo" + juce::String(i + 1) + "_delay"] = { nullptr, TetraOPAudioProcessorEditor::kLFO };
        filteredParams["lfo" + juce::String(i + 1) + "_delay_sync"] = { nullptr, TetraOPAudioProcessorEditor::kLFO };
        filteredParams["lfo" + juce::String(i + 1) + "_rise"] = { nullptr, TetraOPAudioProcessorEditor::kLFO };
        filteredParams["lfo" + juce::String(i + 1) + "_rise_sync"] = { nullptr, TetraOPAudioProcessorEditor::kLFO };
    }

    for (int i = 0; i < globals::MAX_RNDS; ++i) {
        filteredParams["rnd" + juce::String(i + 1) + "_rate"] = { nullptr, TetraOPAudioProcessorEditor::kRand };
        filteredParams["rnd" + juce::String(i + 1) + "_rate_sync"] = { nullptr, TetraOPAudioProcessorEditor::kRand };
    }

    juce::PopupMenu menu;
    menu.addItem(1, "Remove");
    menu.addSeparator();

    std::vector<juce::String> sortedIds;
    std::map<TetraOPAudioProcessorEditor::ParamCategory, std::vector<juce::String>> grouped;
    FxTree fxTree;

    // --- collect + sort ---
    for (auto& [paramId, param] : filteredParams)
        sortedIds.push_back(paramId);

    std::sort(sortedIds.begin(), sortedIds.end());

    // --- helpers ---
    auto getCategoryName = [](TetraOPAudioProcessorEditor::ParamCategory cat)
        {
            switch (cat)
            {
            case TetraOPAudioProcessorEditor::ParamCategory::kGlobal: return "Global";
			case TetraOPAudioProcessorEditor::ParamCategory::kOSCA: return "OSC A";
        	case TetraOPAudioProcessorEditor::ParamCategory::kOSCB: return "OSC B";
        	case TetraOPAudioProcessorEditor::ParamCategory::kOSCC: return "OSC C";
        	case TetraOPAudioProcessorEditor::ParamCategory::kOSCD: return "OSC D";
        	case TetraOPAudioProcessorEditor::ParamCategory::kFilter1: return "Filter 1";
        	case TetraOPAudioProcessorEditor::ParamCategory::kFilter2: return "Filter 2";
            case TetraOPAudioProcessorEditor::ParamCategory::kEnvelope: return "Envelope";
            case TetraOPAudioProcessorEditor::ParamCategory::kLFO: return "LFO";
            case TetraOPAudioProcessorEditor::ParamCategory::kRand: return "Rand";
            case TetraOPAudioProcessorEditor::ParamCategory::kMacro: return "Macro";
            case TetraOPAudioProcessorEditor::ParamCategory::kFX: return "FX";
            case TetraOPAudioProcessorEditor::ParamCategory::kModulation: return "Modulation";
            case TetraOPAudioProcessorEditor::ParamCategory::kOther: return "Other";
            default: return "Other";
            }
        };

    const std::vector<TetraOPAudioProcessorEditor::ParamCategory> categoryOrder = {
        TetraOPAudioProcessorEditor::ParamCategory::kGlobal,
        TetraOPAudioProcessorEditor::ParamCategory::kOSCA,
        TetraOPAudioProcessorEditor::ParamCategory::kOSCB,
        TetraOPAudioProcessorEditor::ParamCategory::kOSCC,
        TetraOPAudioProcessorEditor::ParamCategory::kOSCD,
        TetraOPAudioProcessorEditor::ParamCategory::kFilter1,
        TetraOPAudioProcessorEditor::ParamCategory::kFilter2,
        TetraOPAudioProcessorEditor::ParamCategory::kEnvelope,
        TetraOPAudioProcessorEditor::ParamCategory::kLFO,
        TetraOPAudioProcessorEditor::ParamCategory::kRand,
        TetraOPAudioProcessorEditor::ParamCategory::kMacro,
        TetraOPAudioProcessorEditor::ParamCategory::kFX,
        TetraOPAudioProcessorEditor::ParamCategory::kModulation,
        TetraOPAudioProcessorEditor::ParamCategory::kOther
    };

    auto parseFxParam = [](const juce::String& paramId)
        {
            auto parts = juce::StringArray::fromTokens(paramId, "_", "");

            FxInfo info;

            if (parts.size() < 3)
                return info;

            // Layer
            if (parts[0] == "m")       info.layer = "Master";
            else if (parts[0] == "l1") info.layer = "Layer1";
            else if (parts[0] == "l2") info.layer = "Layer2";
            else                       info.layer = "Other";

            // FX name
            if (parts[1] == "fx" && parts.size() >= 3)
                info.fxName = parts[2];   // stereoizer
            else
                info.fxName = parts[1];   // fallback

            return info;
        };

    // --- build structures ---
    for (auto& id : sortedIds) {
        auto& param = filteredParams[id];

        if (param.cat == TetraOPAudioProcessorEditor::ParamCategory::kFX) {
            auto info = parseFxParam(id);
            fxTree[info.layer][info.fxName].push_back(id);
        }
        else {
            grouped[param.cat].push_back(id);
        }
    }

    const std::vector<juce::String> layerOrder = {
        "Master", "Layer1", "Layer2", "Other"
    };

    auto formatFXName = [](const juce::String& input)
        {
            static const std::map<juce::String, juce::String> aliasMap = {
                {"trem", "Tremolo"},
                {"dist", "Distortion"},
                {"stereo", "Stereoizer"},
                {"comp", "Compressor"},
                {"eq", "EQ"},
            };

            auto it = aliasMap.find(input);
            if (it != aliasMap.end())
                return it->second;

            return UIUtils::capitalize(input);
        };

    int itemId = 2;
    std::map<int, juce::String> idLookup;

    // --- build menu ---
    for (auto category : categoryOrder)
    {
        if (category == TetraOPAudioProcessorEditor::ParamCategory::kFX) {
            if (fxTree.empty())
                continue;

            juce::PopupMenu fxMenu;

            for (auto& layer : layerOrder) {
                auto lit = fxTree.find(layer);
                if (lit == fxTree.end()) continue;

                juce::PopupMenu layerMenu;

                for (auto& [fxName, ids] : lit->second) {
                    juce::PopupMenu fxNameMenu;

                    auto sorted = ids;
                    std::sort(sorted.begin(), sorted.end());

                    for (auto& id : sorted) {
                        idLookup[itemId] = id;
                        fxNameMenu.addItem(itemId++, UIUtils::aliasParameter(id));
                    }

                    layerMenu.addSubMenu(formatFXName(fxName), fxNameMenu);
                }

                fxMenu.addSubMenu(layer, layerMenu);
            }

            menu.addSubMenu("FX", fxMenu);
            continue;
        }

        auto it = grouped.find(category);
        if (it == grouped.end() || it->second.empty())
            continue;

        juce::PopupMenu subMenu;

        for (auto& id : it->second) {
            idLookup[itemId] = id;
            subMenu.addItem(itemId++, UIUtils::aliasParameter(id));
        }

        menu.addSubMenu(getCategoryName(category), subMenu);
    }

    menu.setLookAndFeel(&component->getLookAndFeel());
    menu.showMenuAsync(
        juce::PopupMenu::Options()
        .withParentComponent(component->findParentComponentOfClass<juce::AudioProcessorEditor>())
        .withTargetComponent(component)
        .withTargetScreenArea({ pos.getX(), pos.getY(), 1, 1 }),
        [cb, idLookup](int result)
        {
            if (result == 0) return;

            if (result == 1)
            {
                cb("-");
                return;
            }

            auto it = idLookup.find(result);
            if (it != idLookup.end())
                cb(it->second);
        }
    );
}

// LIST ROW =======================================================================

Row::Row(TetraOPAudioProcessorEditor& e
	, Modulation::Connection _conn
	, Rectangle<float> bid
	, Rectangle<float> bsrc
	, Rectangle<float> bmap
	, Rectangle<float> bpow
	, Rectangle<float> bpolar
	, Rectangle<float> bamount
	, Rectangle<float> bdst
	, Rectangle<float> bpass
	, std::function<void(Modulation::Connection conn)> onClickMap
) : editor(e)
, conn(_conn)
, bid(bid)
, bsrc(bsrc)
, bmap(bmap)
, bpow(bpow)
, bpolar(bpolar)
, bamount(bamount)
, bdst(bdst)
, bpass(bpass)
, onClickMap(onClickMap)
{
	addAndMakeVisible(src);
	src.setAlpha(0.f);

	map = std::make_unique<MapPreview>(conn, onClickMap);
	addAndMakeVisible(map.get());

	curve = std::make_unique<PowerCurve>(editor, String("mod") + String(conn.id) + "_ten", conn.bipolar);
	addAndMakeVisible(curve.get());
	addAndMakeVisible(polar);
	polar.setAlpha(0.f);
	amount = std::make_unique<HSlider>(editor, String("mod") + String(conn.id) + "_amt", "Amount", HSlider::Percent, true, Colours::white);
	editor.registerModParam(amount.get(), TetraOPAudioProcessorEditor::kModulation);
	addAndMakeVisible(amount.get());
	addAndMakeVisible(dst);
	dst.setAlpha(0.f);
	addAndMakeVisible(bypass);
	bypass.setAlpha(0.f);

	src.onClick = [this]()
		{
			auto pos = localPointToGlobal(src.getBounds().getBottomLeft());
			showSourceMenu((Component *)this, pos, [this](String src)
				{
					if (src == "") return;
					if (src == "-") {
						editor.audioProcessor.modulation->disconnect(conn.src, conn.dst);
					}
					else {
						editor.audioProcessor.modulation->changeConnection(conn.src, conn.dst, src, conn.dst);
					}
				});
		};

	dst.onClick = [this]()
		{
			auto pos = localPointToGlobal(dst.getBounds().getBottomLeft());
			showDestinationMenu((Component*)this, editor, pos, [this](String dst)
				{
					if (dst == "") return;
					if (dst == "-") {
						editor.audioProcessor.modulation->disconnect(conn.src, conn.dst);
					}
					else {
						editor.audioProcessor.modulation->changeConnection(conn.src, conn.dst, conn.src, dst);
					}
				});
		};

	polar.onClick = [this]()
		{
			if (!isKeyTrackPitch(conn))
				editor.audioProcessor.modulation->setConnectionBipolar(conn.src, conn.dst, !conn.bipolar);
		};

	bypass.onClick = [this]()
		{
			editor.audioProcessor.modulation->setConnectionBypass(conn.src, conn.dst, !conn.bypass);
		};

	if (isKeyTrackPitch(conn)) {
		curve->setVisible(false);
	}
}

void Row::disconnect()
{
	editor.unregisterModParam(amount.get());
}

bool Row::isKeyTrackPitch(Modulation::Connection c)
{
	return String(c.src).startsWith("key") && String(c.dst).endsWith("pitch");
}

void Row::setConnection(Modulation::Connection _conn)
{
	if (conn.id != _conn.id) {
		if (amount->paramId.isNotEmpty()) {
			disconnect();
		}
		amount->setParamId(String("mod") + String(_conn.id) + "_amt");
		editor.registerModParam(amount.get(), TetraOPAudioProcessorEditor::kModulation);
	}

	conn = _conn;
	if (curve->bipolar != conn.bipolar) {
		curve->bipolar = conn.bipolar;
		curve->repaint();
	}
	auto curveParamId = String("mod") + String(conn.id) + "_ten";
	if (curve->paramId != curveParamId) {
		curve->setParam(curveParamId);
	}
	curve->setVisible(!isKeyTrackPitch(conn));
	map->setConn(conn);

	repaint();
}

void Row::resized()
{
	auto b = getLocalBounds();
	src.setBounds(b.withWidth((int)bsrc.getWidth()).withX((int)bid.getWidth()));
	bmap = b.toFloat().withWidth(bmap.getWidth()).withX((float)src.getRight());
	map->setBounds(bmap.reduced(10.f, 1.f).toNearestInt());
	bpow = b.toFloat().withWidth(bpow.getWidth()).withX((float)bmap.getRight());
	curve->setBounds(bpow.reduced(10.f, 1.0).toNearestInt());
	polar.setBounds(b.withWidth((int)bpolar.getWidth()).withX((int)bpow.getRight()));
	amount->setBounds(b.withWidth((int)bamount.getWidth()).withX(polar.getRight()).reduced(0, 2));
	dst.setBounds(b.withWidth((int)bdst.getWidth()).withX(amount->getRight()));
	bypass.setBounds(b.withWidth((int)bpass.getWidth()).withX(dst.getRight()));

}

void Row::paint(Graphics& g)
{
	g.setColour(Colour(Colours::black.withAlpha(0.25f)));
	g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.f);

	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(FontOptions(16.f));
	g.drawText(String(conn.id), 0, 0, (int)bid.getWidth(), (int)bid.getHeight(), Justification::centredRight);
	g.drawText(UIUtils::aliasModulator(String(conn.src)), src.getBounds(), Justification::centred);
	g.drawText(UIUtils::aliasParameter(String(conn.dst)), dst.getBounds(), Justification::centred);

	if (isEditingMap) {
		g.setColour(COLOR_ACTIVE());
		g.drawRoundedRectangle(map->getBounds().toFloat().reduced(0.5f), 4.f, 1.f);
	}

	if (!isKeyTrackPitch(conn)) {
		UIUtils::drawCheckmark(g, {
				polar.getBounds().getCentreX() - 13 / 2.f,
				polar.getBounds().getCentreY() - 13 / 2.f,
				13.f, 13.f
			}, COLOR_PANEL(), COLOR_ACTIVE(), conn.bipolar);
	}

	UIUtils::drawPowerButton(g, bypass.getBounds().toFloat(),
		conn.bypass ? Colour(0xff666666) : COLOR_ACTIVE(), 1.f);
}

// MATRIX PANEL =============================================================

MatrixPanel::MatrixPanel(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
	addAndMakeVisible(list);
	list.setViewedComponent(&content, false);
	list.setScrollBarsShown(true, false);
	list.setScrollBarThickness(10);
	list.getVerticalScrollBar()
		.setColour(ScrollBar::ColourIds::thumbColourId, Colours::white);

	mapEditor = std::make_unique<MapEditor>(editor, [this]
		{
			mapEditor->setVisible(false);
			onEditMapChange();
			toggleUIComponents();
		});
	addChildComponent(mapEditor.get());

	addAndMakeVisible(newBtn);
	newBtn.setAlpha(0.f);
	newBtn.onClick = [this]()
		{
			auto pos = localPointToGlobal(newBtn.getBounds().getBottomLeft());
			showSourceMenu((Component*)this, pos, [this, pos](String src)
				{
					if (src == "" || src == "-") return;
					showDestinationMenu((Component*)this, editor, pos, [this, src](String dst)
						{
							if (dst == "" || dst == "-") return;
							editor.audioProcessor.modulation->connect(src, dst);
						});
				});
		};
}

MatrixPanel::~MatrixPanel()
{
}

void MatrixPanel::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
}

void MatrixPanel::setConnections(std::vector<Modulation::Connection> _conns)
{
	// update rows connections
	if (conns.size() != _conns.size()) {
		while (rows.size()) {
			auto* row = rows.removeAndReturn(0);
			row->disconnect();
			content.removeChildComponent(row);
			delete row;
		}
		for (int i = 0; i < _conns.size(); ++i) {
			auto* row = new Row(editor, _conns[i], bid, bsrc, bmap, bpow, bpolar, bamount, bdst, bpass,
				[this](Modulation::Connection conn)
				{
					// close map editor if the user clicked the map preview again
					if (mapEditor->isVisible() && mapEditor->conn.src == conn.src && mapEditor->conn.dst == conn.dst) {
						Modulation::Connection c;
						mapEditor->setConnection(c);
						mapEditor->setVisible(false);
					}
					// set new connection and show map editor
					else {
						mapEditor->setConnection(conn);
						mapEditor->setVisible(true);
					}
					onEditMapChange();
					toggleUIComponents();
				});
			rows.add(row);
			content.addAndMakeVisible(row);
		}
		resized();
	}
	else {
		for (int i = 0; i < _conns.size(); ++i) {
			rows[i]->setConnection(_conns[i]);
		}
	}

	// update map editor connection
	if (mapEditor->isVisible()) {
		bool found = false;
		for (auto& conn : _conns) {
			if (conn.src == mapEditor->conn.src && conn.dst == mapEditor->conn.dst) {
				mapEditor->setConnection(conn);
				found = true;
				break;
			}
		}
		if (!found) {
			mapEditor->setVisible(false);
			toggleUIComponents();
		}
	}

	onEditMapChange();
}

// highlight the rows map being edited
void MatrixPanel::onEditMapChange()
{
	auto mapVisible = mapEditor->isVisible();
	auto& mapConn = mapEditor->conn;
	for (auto& row : rows) {
		bool isEditingMap = mapVisible &&
			mapConn.src == row->conn.src &&
			mapConn.dst == row->conn.dst;

		row->isEditingMap = isEditingMap;
		row->repaint();
	}
}

void MatrixPanel::paint(Graphics& g)
{
	auto bounds = getLocalBounds().toFloat();
	UIUtils::drawPanel(g, bounds, false);

	g.setColour(COLOR_BEVEL());
	g.fillRoundedRectangle(bid.withRight(bpass.getRight()), 4.f);
	g.setColour(COLOR_PANEL());
	g.drawLine(bid.getRight(), bid.getY(), bid.getRight(), bid.getBottom(), 2.f);
	g.drawLine(bsrc.getRight(), bsrc.getY(), bsrc.getRight(), bsrc.getBottom(), 2.f);
	g.drawLine(bmap.getRight(), bsrc.getY(), bmap.getRight(), bsrc.getBottom(), 2.f);
	g.drawLine(bpow.getRight(), bsrc.getY(), bpow.getRight(), bsrc.getBottom(), 2.f);
	g.drawLine(bpolar.getRight(), bsrc.getY(), bpolar.getRight(), bsrc.getBottom(), 2.f);
	g.drawLine(bamount.getRight(), bsrc.getY(), bamount.getRight(), bsrc.getBottom(), 2.f);
	g.drawLine(bdst.getRight(), bsrc.getY(), bdst.getRight(), bsrc.getBottom(), 2.f);

	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(16.f);
	g.drawText("ID", bid, Justification::centred);
	g.drawText("SOURCE", bsrc, Justification::centred);
	g.drawText("MAP", bmap, Justification::centred);
	g.drawText("POW", bpow, Justification::centred);
	g.drawText("BPOL", bpolar, Justification::centred);
	g.drawText("AMOUNT", bamount, Justification::centred);
	g.drawText("DESTINATION", bdst, Justification::centred);
	//g.drawText("", bpass, Justification::centred);

	Path p;
	auto b = newBtn.getBounds().toFloat().reduced(6.f);
	p.startNewSubPath(b.getX(), b.getCentreY());
	p.lineTo(b.getRight(), b.getCentreY());
	p.startNewSubPath(b.getCentreX(), b.getY());
	p.lineTo(b.getCentreX(), b.getBottom());
	g.setColour(COLOR_ACTIVE());
	g.strokePath(p, PathStrokeType(2.f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
}

void MatrixPanel::resized()
{
	auto bounds = getLocalBounds();
	auto scroll = list.getViewPosition();

	bid = { 0.f, 0.f, 25.f, 25.f };
	bid.translate(margin, margin);
	bsrc = { 0.f, 0.f, 150.f, 25.f };
	bsrc.translate(bid.getRight(), margin);
	bmap = { 0.f, 0.f, 50.f, 25.f };
	bmap.translate(bsrc.getRight(), margin);
	bpow = { 0.f, 0.f, 50.f, 25.f };
	bpow.translate(bmap.getRight(), margin);
	bpolar = { 0.f, 0.f, 50.f, 25.f };
	bpolar.translate(bpow.getRight(), margin);
	bamount = { 0.f, 0.f, 170.f, 25.f };
	bamount.translate(bpolar.getRight(), margin);
	bdst = { 0.f, 0.f, 150.f, 25.f };
	bdst.translate(bamount.getRight(), margin);
	bpass = { 0.f, 0.f, 35.f, 25.f };
	bpass.translate(bdst.getRight(), margin);

	auto bheader = bid.withRight(bpass.getRight()).toNearestInt();
	float translateX = bounds.getCentreX() - bheader.getWidth() / 2.f - bheader.getX();

	bheader.translate(translateX, 0);
	bid.translate(translateX, 0);
	bsrc.translate(translateX, 0);
	bmap.translate(translateX, 0);
	bpow.translate(translateX, 0);
	bpolar.translate(translateX, 0);
	bamount.translate(translateX, 0);
	bdst.translate(translateX, 0);
	bpass.translate(translateX, 0);

	newBtn.setBounds(bpass.withX(bpass.getX() + 5).withWidth(bpass.getHeight()).toNearestInt());

	mapEditor->setBounds(bid
		.withRight(bpass.getRight()).toNearestInt()
		.translated(0, (int)bid.getHeight())
		.withBottom((int)(getBottom() - 50))
		.withTop(bounds.getCentreY()));

	list.setBounds(bid
		.withRight(bpass.getRight() + 10).toNearestInt()
		.translated(0, (int)bid.getHeight())
		.withBottom((int)(getBottom() - 50)));

	if (mapEditor->isVisible()) {
		list.setBounds(list.getBounds().withBottom(bounds.getCentreY() - 5));
	}

	for (int i = 0; i < rows.size(); ++i)
		rows[i]->setBounds(0, i * (25 + 2) + 2, bheader.getWidth(), 25);

	content.setBounds(list.getBounds().withHeight((int)rows.size() * (25 + 2)));

	list.setViewPosition(scroll);
}

void MatrixPanel::toggleUIComponents()
{
	resized();
}