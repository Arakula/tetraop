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

		fx->onDrag = [this](UIFX* fx) { onDrag(fx); };
		fx->onDragEnded = [this](UIFX* fx) { onDragEnded(fx); };

		fxs.add(fx);
		addAndMakeVisible(fx);
	}

	refreshFX(false);
	onFXChanged(false);
}

FXPanel::~FXPanel()
{
	for (int i = 0; i < FX::kFXs; ++i) {
		auto prefix = juce::String(FX::FXPrefix[i].data());
		editor.audioProcessor.params.removeParameterListener(prefix + "on", this);
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
void FXPanel::refreshFX(bool)
{
	for (int i = 0; i < FX::kFXs; ++i) {
		auto prefix = juce::String(FX::FXPrefix[i].data());
		bool on = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "on")->load();

        for (int j = 0; j < fxs.size(); ++j)
        {
            auto* fx = fxs[j];
            if (fx->type == i) {
				fx->setAlpha(!on ? 0.5f : 1.f);
				fx->on = on;
				fx->onActiveToggle();
                fx->repaint();
                break;
            }
        }
	}

	resized();
}

/*
* Internal FXs lists sort based on audio thread fxchain (source of truth)
*/
void FXPanel::onFXChanged(bool)
{
	std::unordered_map<int, size_t> position;
	auto& fxOrder = editor.audioProcessor.fxOrder;
	for (size_t i = 0; i < fxOrder.size(); i++)
		position[fxOrder[i]] = i;

	// Sort fxs based on processor FX order
	std::sort(fxs.begin(), fxs.end(),
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

	resized();
}

void FXPanel::paint(juce::Graphics& g)
{
	g.fillAll(COLOR_PANEL().darker(0.5f));
	g.setFont(juce::FontOptions(16.f));
}

void FXPanel::resized()
{
	for (int i = 0; i < fxs.size(); ++i) {
		auto* fx = fxs[i];
		fx->setBounds(PANEL_PAD + i * (KNOB_WIDTH * 2 + PANEL_PAD), PANEL_PAD, KNOB_WIDTH * 2, getHeight() - PANEL_PAD * 2);
	}

	toggleUIComponents();
}

void FXPanel::onDrag(UIFX* dragged)
{
	int draggedIndex = 0;

	for (int i = 0; fxs[i]->type != dragged->type; ++i)
		draggedIndex = i;

	// find gap
	auto itemWidth = fxs[0]->getBounds().getWidth();
	int draggedX = dragged->getX();
	int gapIndex = 0;
	for (int i = 0; i < int (fxs.size()); ++i)
	{
		if (draggedX + itemWidth / 2 >= PANEL_PAD + i * (itemWidth + PANEL_PAD))
			gapIndex = draggedIndex < i ? i + 1 : i;
	}

	// set positions
	int x = PANEL_PAD;
	for (int i = 0; i < int (fxs.size()); ++i)
	{
		if (fxs[i]->type == dragged->type) continue;
		if (i == gapIndex)
			x += itemWidth + PANEL_PAD; // create gap

		fxs[i]->setTopLeftPosition(x, PANEL_PAD);
		x += itemWidth + PANEL_PAD;
	}
}

void FXPanel::onDragEnded(UIFX* header)
{
	(void)header;
	// Re-sort items vector based on X position
	std::sort(fxs.begin(), fxs.end(),
		[](const auto& a, const auto& b) { return a->getX() < b->getX(); });

	int x = PANEL_PAD;
	auto itemWidth = fxs[0]->getBounds().getWidth();
	for (auto& it : fxs) {
		it->setTopLeftPosition(x, PANEL_PAD);
		x += itemWidth + PANEL_PAD;
	}

	std::vector<FX::FXType> sorted{};
	for (auto& h : fxs) {
		sorted.push_back(h->type);
	}
	editor.audioProcessor.sortFX(sorted);
}

void FXPanel::toggleUIComponents()
{
	repaint();
}
