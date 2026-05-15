#include "FXPanel.h"
#include "../../PluginEditor.h"

FXPanel::FXPanel(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
	startTimerHz(30);

	for (int i = 0; i < FX::kFXs; ++i) {
		auto prefix = juce::String(FX::FXPrefix[i].data());
		editor.audioProcessor.params.addParameterListener(prefix + "on", this);

		UIFX* fx;

		if (i == FX::Compressor) fx = new FXCompressor(editor);
		else if (i == FX::Distortion) fx = new FXDist(editor);
		else if (i == FX::Chorus) fx = new FXChorus(editor);
		else if (i == FX::Delay) fx = new FXDelay(editor);
		else if (i == FX::Reverb) fx = new FXReverb(editor);
		else if (i == FX::EQ) fx = new FXEQ(editor);
		else if (i == FX::Phaser) fx = new FXPhaser(editor);
		else continue;

		fx->onDrag = [this](FXHeader* header) { onDrag(header); };
		fx->onDragEnded = [this](FXHeader* header) { onDragEnded(header); };

		auto& fxheaders = layer == 0 ? fxheadersM : layer == 1 ? fxheadersL1 : fxheadersL2;
		auto& fxs = layer == 0 ? fxsM : layer == 1 ? fxsL1 : fxsL2;
		auto& content = layer == 0 ? contentM : layer == 1 ? contentL1 : contentL2;

		addChildComponent(*fxheader);
		fxheaders.push_back(std::move(fxheader));
		fxs.add(fx);
		content.addChildComponent(fx);
	}

	editor.audioProcessor.fxlayer = editor.audioProcessor.instance.selectedFxBank;
	refreshFX(false);
	onFXChanged(false);

	fxlistM.setViewPosition(0, editor.audioProcessor.instance.fxScrollM);
	fxlistL1.setViewPosition(0, editor.audioProcessor.instance.fxScrollL1);
	fxlistL2.setViewPosition(0, editor.audioProcessor.instance.fxScrollL2);
}

FXPanel::~FXPanel()
{
    editor.audioProcessor.instance.fxScrollM = fxlistM.getViewPositionY();
    editor.audioProcessor.instance.fxScrollL1 = fxlistL1.getViewPositionY();
    editor.audioProcessor.instance.fxScrollL2 = fxlistL2.getViewPositionY();

    editor.audioProcessor.params.removeParameterListener("fx_bypass_master", this);
    editor.audioProcessor.params.removeParameterListener("fx_bypass_l1", this);
    editor.audioProcessor.params.removeParameterListener("fx_bypass_l2", this);

	for (int layer = 0; layer < 3; ++layer) {
		auto prel = layer == 0 ? "m_" : layer == 1 ? "l1_" : "l2_";
		for (int i = 0; i < FX::kFXs; ++i) {
			if (layer == 0 && i == FX::Vibrato) continue;
			auto prefix = juce::String(FX::FXPrefix[i].data());
			editor.audioProcessor.params.removeParameterListener(prel + prefix + "on", this);
			editor.audioProcessor.params.removeParameterListener(prel + prefix + "bypass", this);
		}
	}
}

/*
* To make it thread safe the Effects panel and audio thread
* synchronize using polling for GUI updates
* and defered sortFX() calls for fx sorting
*/
void FXPanel::timerCallback()
{
	if (isVisible() && editor.audioProcessor.FXDirty.load()) {
		editor.audioProcessor.FXDirty.store(false);
		onFXChanged();
		resized();
	}
}

void FXPanel::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
	auto safeThis = juce::Component::SafePointer(this);
	juce::MessageManager::callAsync([safeThis]()
		{
			if (auto* self = safeThis.getComponent())
				self->refreshFX();
		});
}

/*
* Called whenever there is a on/off or bypass FX toggle
*
* Initially this was done on the component level
* However its harder to debug, this solution is more inefficient
* but easier to maintain.
*/
void FXPanel::refreshFX(bool scrollViewport)
{
	auto layer = editor.audioProcessor.fxlayer;
    bool sameLayer = prevLayer == layer;
    prevLayer = layer;

	auto& viewport = layer == 0 ? fxlistM : layer == 1 ? fxlistL1 : fxlistL2;
	auto scroll = viewport.getViewPositionY();

    int scrollToFx = -1; // scroll viewport to newly added fx
	juce::String prel = layer == 0 ? "m_" : layer == 1 ? "l1_" : "l2_";

	for (int i = 0; i < FX::kFXs; ++i) {
		if (layer == 0 && i == FX::Vibrato) continue;
        if (layer != 0 && i == FX::Limiter) continue;
		auto& fxheaders = layer == 0 ? fxheadersM : layer == 1 ? fxheadersL1 : fxheadersL2;
		auto& fxs = layer == 0 ? fxsM : layer == 1 ? fxsL1 : fxsL2;

		auto prefix = prel + juce::String(FX::FXPrefix[i].data());
		bool on = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "on")->load();
		bool bypass = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "bypass")->load();
        juce::String bypassLayerParam = layer == 0 ? "fx_bypass_master" : layer == 1 ? "fx_bypass_l1" : "fx_bypass_l2";
        bool bypassLayer = (bool)editor.audioProcessor.params.getRawParameterValue(bypassLayerParam)->load();

		for (auto& header : fxheaders) {
			if (header->type == i) {
				header->onBtn.setToggleState(on, juce::dontSendNotification);
				break;
			}
		}

        for (int j = 0; j < fxs.size(); ++j)
        {
            auto* fx = fxs[j];
            fx->setAlpha(bypassLayer ? 0.5f : 1.f);
            if (fx->type == i) {
                if (sameLayer && !fx->isVisible() && on)
                    scrollToFx = j;
                fx->bypass = bypass;
                fx->setVisible(on);
                fx->repaint();
                break;
            }
        }
	}

	resized();
	if (scrollViewport) {
		auto safeViewport = juce::Component::SafePointer(&viewport);
		juce::MessageManager::callAsync([safeViewport, scroll]
			{
				if (auto* vp = safeViewport.getComponent())
					vp->setViewPosition(0, scroll);
			});
	}

    if (scrollToFx > -1) {
        auto& fxs = layer == 0 ? fxsM : layer == 1 ? fxsL1 : fxsL2;
        auto safeFx = juce::Component::SafePointer(fxs[scrollToFx]);
        auto safeViewport = juce::Component::SafePointer(&viewport);
        juce::MessageManager::callAsync([safeViewport, safeFx]
            {
                if (auto* vp = safeViewport.getComponent())
                    if (auto* fx = safeFx.getComponent())
                        vp->setViewPosition(0, fx->getBounds().getY());
            });
    }
}

/*
* Internal FXs lists sort based on audio thread fxchain (source of truth)
*/
void FXPanel::onFXChanged(bool scrollViewport)
{
	auto& layer = editor.audioProcessor.fxlayer;
	auto& viewport = layer == 0 ? fxlistM : layer == 1 ? fxlistL1 : fxlistL2;
	auto scroll = viewport.getViewPositionY();

	std::unordered_map<int, size_t> position;
	auto& fxOrder = editor.audioProcessor.getFXOrderForLayer(layer);
	for (size_t i = 0; i < fxOrder.size(); i++)
		position[fxOrder[i]] = i;

	// Sort fx headers based on processor FX order
	auto& fxheaders = layer == 0 ? fxheadersM
		: layer == 1 ? fxheadersL1 : fxheadersL2;

	std::sort(fxheaders.begin(), fxheaders.end(),
		[&](const auto& a, const auto& b)
		{
			int typeA = a->type;
			int typeB = b->type;

			bool aExists = position.count(typeA) > 0;
			bool bExists = position.count(typeB) > 0;

			if (!aExists && !bExists) return typeA < typeB;
			if (!aExists) return false;
			if (!bExists) return true;

			return position[typeA] < position[typeB];
		});

	std::unordered_map<int, size_t> headerpos;
	for (size_t i = 0; i < fxheaders.size(); ++i)
		headerpos[fxheaders[i]->type] = i;

	// Sort fxs based on header position
	auto& fxs = layer == 0 ? fxsM : layer == 1 ? fxsL1 : fxsL2;

	std::sort(fxs.begin(), fxs.end(),
		[&](UIFX* a, UIFX* b)
		{
			int typeA = a->type;
			int typeB = b->type;

			bool aExists = headerpos.count(typeA) > 0;
			bool bExists = headerpos.count(typeB) > 0;

			if (!aExists && !bExists) return false;
			if (!aExists) return false;
			if (!bExists) return true;

			return headerpos[typeA] < headerpos[typeB];
		});

	resized();
	if (scrollViewport) {
		auto safeViewport = juce::Component::SafePointer(&viewport);
		juce::MessageManager::callAsync([safeViewport, scroll]
			{
				if (auto* vp = safeViewport.getComponent())
					vp->setViewPosition(0, scroll);
			});
	}
}

void FXPanel::paint(juce::Graphics& g)
{
	g.fillAll(theme.COLOR_MAIN_BACKGROUND());
	UIUtils::drawPanel(g, lcol, false, theme);
	UIUtils::drawPanel(g, rcol.withHeight(35.f), false, theme);

	auto layer = editor.audioProcessor.fxlayer;
	g.setFont(juce::FontOptions(16.f));

	g.setColour(theme.COLOR_ACTIVE_CONTRAST());
	if (layer == 1) {
		g.fillRoundedRectangle(layer1Btn.getBounds().toFloat(), 2.f);
		g.setColour(theme.COLOR_PANEL());
		g.drawText("LAYER 1", layer1Btn.getBounds().toFloat(), juce::Justification::centred);
	}
	else {
		g.drawText("LAYER 1", layer1Btn.getBounds().toFloat(), juce::Justification::centred);
	}

	g.setColour(theme.COLOR_ACTIVE_ALT_CONTRAST());
	if (layer == 2) {
		g.fillRoundedRectangle(layer2Btn.getBounds().toFloat(), 2.f);
		g.setColour(theme.COLOR_PANEL());
		g.drawText("LAYER 2", layer2Btn.getBounds().toFloat(), juce::Justification::centred);
	}
	else {
		g.drawText("LAYER 2", layer2Btn.getBounds().toFloat(), juce::Justification::centred);
	}

	g.setColour(theme.COLOR_FX_MASTER());
	if (layer == 0) {
		g.fillRoundedRectangle(masterBtn.getBounds().toFloat(), 2.f);
		g.setColour(theme.COLOR_PANEL());
		g.drawText("MASTER", masterBtn.getBounds().toFloat(), juce::Justification::centred);
	}
	else {
		g.drawText("MASTER", masterBtn.getBounds().toFloat(), juce::Justification::centred);
	}

    bool bypassMaster = (bool)editor.audioProcessor.params.getRawParameterValue("fx_bypass_master")->load();
    bool bypassL1 = (bool)editor.audioProcessor.params.getRawParameterValue("fx_bypass_l1")->load();
    bool bypassL2 = (bool)editor.audioProcessor.params.getRawParameterValue("fx_bypass_l2")->load();

    UIUtils::drawPowerButton(g, masterBypassBtn.getBounds().toFloat(), bypassMaster ? theme.COLOR_MAIN_BACKGROUND() : theme.COLOR_FX_MASTER().darker(0.5f));
    UIUtils::drawPowerButton(g, layer1BypassBtn.getBounds().toFloat(), bypassL1 ? theme.COLOR_MAIN_BACKGROUND() : theme.COLOR_ACTIVE_CONTRAST().darker(0.5f));
    UIUtils::drawPowerButton(g, layer2BypassBtn.getBounds().toFloat(), bypassL2 ? theme.COLOR_MAIN_BACKGROUND() : theme.COLOR_ACTIVE_ALT_CONTRAST().darker(0.5f));
}

void FXPanel::resized()
{
	// Paint-only column rectangles, kept in C++.
	auto b = getLocalBounds().toFloat();
	lcol = b.withWidth(170.f);
	rcol = b.withTrimmedLeft(170.f + theme.PANEL_MARGIN);

   #if JUCE_DEBUG
	layout.setLayout ({ global::getLayoutDirectory().getChildFile ("layout_effects.json") });
   #else
	layout.setLayout (juce::StringArray ("layout_effects.json"));
   #endif

	// Dynamic: per-layer fx headers/list/content sized from the live FX list.
	auto lfxh = 30.f;
	auto brow = lcol.reduced((float)theme.PANEL_MARGIN).withHeight(lfxh);

	auto layer = editor.audioProcessor.fxlayer;
	auto& fxheaders = layer == 0 ? fxheadersM : layer == 1 ? fxheadersL1 : fxheadersL2;
	for (int i = 0; i < int (fxheaders.size()); ++i) {
		auto& lfx = fxheaders[i];
		lfx->setBounds(brow.withY(brow.getY() + i * (lfxh + theme.FX_HEADER_PAD)).toNearestInt());
	}

	auto& fxlist = layer == 0 ? fxlistM : layer == 1 ? fxlistL1 : fxlistL2;

	auto& fxs = layer == 0 ? fxsM : layer == 1 ? fxsL1 : fxsL2;
	int contentHeight = 0;
	for (int i = 0; i < fxs.size(); ++i) {
		auto* fx = fxs[i];
		if (fx->isVisible()) {
			auto currentFxh = fx->getPreferredHeight();
			fx->setBounds(0, contentHeight, (int)rcol.getWidth(), currentFxh);
			contentHeight += currentFxh + theme.PANEL_MARGIN;
		}
	}

	auto& content = layer == 0 ? contentM : layer == 1 ? contentL1 : contentL2;
	content.setBounds(fxlist.getBounds().withHeight(contentHeight));
	toggleUIComponents();
}

void FXPanel::onDrag(FXHeader* dragged)
{
	int draggedIndex = 0;
	auto layer = editor.audioProcessor.fxlayer;
	auto& fxheaders = layer == 0 ? fxheadersM : layer == 1 ? fxheadersL1 : fxheadersL2;

	for (int i = 0; fxheaders[i].get() != dragged; ++i)
		draggedIndex = i;

	// find gap
	auto itemHeight = fxheaders[0]->getBounds().getHeight();
	int draggedY = dragged->getY();
	int gapIndex = 0;
	for (int i = 0; i < int (fxheaders.size()); ++i)
	{
		if (draggedY + itemHeight / 2 >= theme.FX_HEADER_PAD + i * (itemHeight + theme.FX_HEADER_PAD))
			gapIndex = draggedIndex < i ? i + 1 : i;
	}

	// set positions
	int y = theme.PANEL_MARGIN;
	for (int i = 0; i < int (fxheaders.size()); ++i)
	{
		if (fxheaders[i].get() == dragged) continue;
		if (i == gapIndex)
			y += itemHeight + theme.FX_HEADER_PAD; // create gap

		fxheaders[i]->setTopLeftPosition(theme.PANEL_MARGIN, y);
		y += itemHeight + theme.FX_HEADER_PAD;
	}
}

void FXPanel::onDragEnded(FXHeader* header)
{
	(void)header;
	auto layer = editor.audioProcessor.fxlayer;
	auto& fxheaders = layer == 0 ? fxheadersM : layer == 1 ? fxheadersL1 : fxheadersL2;
	// Re-sort items vector based on Y position
	std::sort(fxheaders.begin(), fxheaders.end(),
		[](const auto& a, const auto& b) { return a->getY() < b->getY(); });

	int y = theme.PANEL_MARGIN;
	auto itemHeight = fxheaders[0]->getBounds().getHeight();
	for (auto& it : fxheaders) {
		it->setTopLeftPosition(theme.PANEL_MARGIN, y);
		y += itemHeight + theme.FX_HEADER_PAD;
	}

	std::vector<FX::FXType> sorted{};
	for (auto& h : fxheaders) {
		sorted.push_back(h->type);
	}
	editor.audioProcessor.sortFX(sorted, layer);
}

void FXPanel::toggleUIComponents()
{
	int layer = editor.audioProcessor.fxlayer;
	fxlistM.setVisible(layer == 0);
	fxlistL1.setVisible(layer == 1);
	fxlistL2.setVisible(layer == 2);

	for (auto& header : fxheadersM) {
		header->setVisible(layer == 0);
	}

	for (auto& header : fxheadersL1) {
		header->setVisible(layer == 1);
	}

	for (auto& header : fxheadersL2) {
		header->setVisible(layer == 2);
	}

	repaint();
}
