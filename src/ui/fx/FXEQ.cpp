#include "FXEQ.h"
#include "../../PluginEditor.h"

FXEQ::FXEQ(TetraOPAudioProcessorEditor& e)
	: UIFX(e, FX::EQ)
{
	startTimerHz(30);
	for (int i = 0; i < globals::EQ_BANDS; ++i) {
		auto pre = juce::String("fx_eq_band") + juce::String(i + 1);
		auto freq = std::make_unique<Rotary>(editor, pre + "_freq", "Freq", Rotary::Hz);
		auto q = std::make_unique<Rotary>(editor, pre + "_q", "Q", Rotary::float1);
		auto gain = std::make_unique<Rotary>(editor, pre + "_gain", "Gain", Rotary::dB, true);
		freq->setName ("freq" + juce::String (i));
		q->setName    ("q"    + juce::String (i));
		gain->setName ("gain" + juce::String (i));
		addChildComponent(freq.get());
		addChildComponent(q.get());
		addChildComponent(gain.get());
		editor.registerModParam(freq.get(), TetraOPAudioProcessorEditor::kFX);
		editor.registerModParam(q.get(), TetraOPAudioProcessorEditor::kFX);
		editor.registerModParam(gain.get(), TetraOPAudioProcessorEditor::kFX);
		freqknobs.push_back(std::move(freq));
		qknobs.push_back(std::move(q));
		gainknobs.push_back(std::move(gain));
	}

	bandBtn.setName ("bandBtn");
	addAndMakeVisible(bandBtn);
	bandBtn.setAlpha(0.f);
	bandBtn.onClick = [this]
		{
			showBandModeMenu();
		};
}

FXEQ::~FXEQ()
{
}

void FXEQ::timerCallback()
{
	if (isShowing()) {
		repaint();
	}
}

void FXEQ::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
	juce::MessageManager::callAsync([this] { repaint(); });
}

void FXEQ::mouseDown(const juce::MouseEvent& e)
{
	dragband = -1;
	for (int i = 0; i < globals::EQ_BANDS; ++i) {
		auto& bounds = bandBounds[i];
		if (bounds.contains((float)e.x, (float)e.y)) {
			dragband = i;
			break;
		}
	}

	if (dragband == -1)
	{
		UIFX::mouseDown(e);
		return;
	}

    editor.audioProcessor.undomgr->createUndo();

	selband = dragband;
	freqknobs[selband]->forceLabelShowValue = true;
	gainknobs[selband]->forceLabelShowValue = true;
	freqknobs[selband]->repaint();
	gainknobs[selband]->repaint();

	mouse_down = true;
	UIUtils::startUnboundedMouse(*this, e, editor.audioProcessor.unboundedMouse);
	auto pre = "fx_eq_band" + juce::String(selband + 1);
	cur_freq_normed_value = editor.audioProcessor.params.getParameter(pre + "_freq")->getValue();
	cur_gain_normed_value = editor.audioProcessor.params.getParameter(pre + "_gain")->getValue();
	cur_q_normed_value = editor.audioProcessor.params.getParameter(pre + "_q")->getValue();
	start_mouse_pos = juce::Desktop::getInstance().getMousePosition();
	last_mouse_pos = e.getPosition();
	toggleUIComponents();
}

void FXEQ::mouseUp(const juce::MouseEvent& e)
{
	if (!mouse_down)
	{
		UIFX::mouseUp(e);
		return;
	}
    freqknobs[selband]->forceLabelShowValue = false;
    gainknobs[selband]->forceLabelShowValue = false;
    freqknobs[selband]->repaint();
    gainknobs[selband]->repaint();

    mouse_down = false;
    if (UIUtils::stopUnboundedMouse(*this, e, editor.audioProcessor.unboundedMouse))
    {
        auto pt = localPointToGlobal(bandBounds[selband].getCentre()).toInt();
        juce::Desktop::getInstance().setMousePosition(pt);
    }
	repaint();
}

void FXEQ::mouseDrag(const juce::MouseEvent& e)
{
	if (!mouse_down)
	{
		UIFX::mouseDrag(e);
		return;
	}

	freqknobs[selband]->repaint();
	gainknobs[selband]->repaint();

	auto change = e.getPosition() - last_mouse_pos;
	last_mouse_pos = e.getPosition();
	auto speed = (e.mods.isShiftDown() ? 40.0f : 4.0f) * 100.f;
	auto changeX = change.getX() / speed;
	auto changeY = change.getY() / speed;

	if (e.mods.isCommandDown()) {
		cur_q_normed_value += changeY;
		cur_q_normed_value = std::clamp(cur_q_normed_value, 0.f, 1.f);
	}
	else {
		cur_freq_normed_value += changeX;
		cur_gain_normed_value -= changeY * 2.f;
		cur_freq_normed_value = std::clamp(cur_freq_normed_value, 0.f, 1.f);
		cur_gain_normed_value = std::clamp(cur_gain_normed_value, 0.f, 1.f);
	}

	auto pre = "fx_eq_band" + juce::String(selband + 1);
	if (e.mods.isCommandDown()) {
		editor.audioProcessor.params.getParameter(pre + "_q")->setValueNotifyingHost(cur_q_normed_value);
	}
	else {
		editor.audioProcessor.params.getParameter(pre + "_freq")->setValueNotifyingHost(cur_freq_normed_value);
		editor.audioProcessor.params.getParameter(pre + "_gain")->setValueNotifyingHost(cur_gain_normed_value);
	}

	repaint();
}

void FXEQ::mouseDoubleClick(const juce::MouseEvent& e)
{
    editor.audioProcessor.undomgr->createUndo();

	for (int i = 0; i < globals::EQ_BANDS; ++i) {
		auto& bounds = bandBounds[i];
		if (bounds.contains((float)e.x, (float)e.y)) {
			auto pre = "fx_eq_band" + juce::String(i + 1);
			if (e.mods.isCommandDown()) {
				auto param = editor.audioProcessor.params.getParameter(pre + "_q");
				param->setValueNotifyingHost(param->getDefaultValue());
			}
			else {
				auto param = editor.audioProcessor.params.getParameter(pre + "_freq");
				param->setValueNotifyingHost(param->getDefaultValue());
				param = editor.audioProcessor.params.getParameter(pre + "_gain");
				param->setValueNotifyingHost(param->getDefaultValue());
			}
			repaint();
			break;
		}
	}
}

void FXEQ::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    editor.audioProcessor.undomgr->createUndo();
	auto speed = e.mods.isShiftDown() ? UIUtils::WHEEL_SPEED_FINE : UIUtils::WHEEL_SPEED;

	for (int i = 0; i < globals::EQ_BANDS; ++i) {
		auto& bounds = bandBounds[i];
		if ((mouse_down && selband == i) || (!mouse_down && bounds.contains((float)e.x, (float)e.y))) {
			auto pre = "fx_eq_band" + juce::String(i + 1);
			auto param = editor.audioProcessor.params.getParameter(pre + "_q");
			auto slider_change = UIUtils::wheelChange(wheel, param, wheelAccum, speed);
			if (slider_change == 0.f) return;
			param->beginChangeGesture();
			param->setValueNotifyingHost(param->getValue() + slider_change);
			while (slider_change > 0.0f && param->getValue() == 0.0f) { // FIX wheel not working when value is zero due to param skew
				slider_change += UIUtils::WHEEL_SPEED;
				param->setValueNotifyingHost(param->getValue() + slider_change);
			}
			param->endChangeGesture();
			repaint();
			break;
		}
	}
}

void FXEQ::paint(juce::Graphics& g)
{
	UIFX::paint(g);
	UIUtils::drawBevel(g, viewBounds.expanded(5.5f), 3.f, COLOR_BEVEL());

	std::array<float, globals::EQ_BANDS> freqs{};
	std::array<float, globals::EQ_BANDS> gains{};
	std::array<float, globals::EQ_BANDS> qs{};

	// draw eq
	g.setColour(COLOR_EQ());
	updateEQCurve();
	juce::Path p;
	auto pixels = viewBounds.getWidth();
	for (int i = 0; i < pixels; ++i) {
		float mag = magPoints[i];
		float dB = 20.0f * std::log10(mag);
		float y = std::clamp((globals::EQ_MAX_GAIN - dB) / (2.f * globals::EQ_MAX_GAIN), 0.f, 1.f);
		if (isnan(y)) y = 0.f;
		y = viewBounds.getY() + y * viewBounds.getHeight();
		float x = viewBounds.getX() + i;

		if (i == 0) {
			p.startNewSubPath(x, y);
		}
		else {
			p.lineTo(x, y);
		}
	}
	g.strokePath(p, juce::PathStrokeType(2.f));
	p.lineTo(viewBounds.getX() + viewBounds.getWidth(), viewBounds.getCentreY());
	p.lineTo(viewBounds.getX(), viewBounds.getCentreY());
	p.closeSubPath();
	g.setColour(COLOR_EQ().withAlpha(0.1f));
	g.fillPath(p);

	// draw points
	g.setFont(12.f);
	for (int i = 0; i < globals::EQ_BANDS; ++i) {
		auto pre = "fx_eq_band" + juce::String(i + 1);
		auto freq = editor.audioProcessor.params.getRawParameterValue(pre + "_freq")->load();
		auto gain = editor.audioProcessor.params.getRawParameterValue(pre + "_gain")->load();
		auto q = editor.audioProcessor.params.getRawParameterValue(pre + "_q")->load();

		freqs[i] = freq;
		gains[i] = gain;
		qs[i] = q;

		auto xnorm = (log(freq) - log(20.f)) / (log(20000.f) - log(20.f));
		auto ynorm = (gain + globals::EQ_MAX_GAIN) / (globals::EQ_MAX_GAIN * 2.f);

		auto r = 6.f;
		auto x = viewBounds.getX() + viewBounds.getWidth() * xnorm;
		auto y = viewBounds.getBottom() - viewBounds.getHeight() * ynorm;

		bandBounds[i] = { x - r, y - r, r * 2, r * 2 };
		g.setColour(COLOR_EQ().withAlpha(selband == i ? 1.f : 0.5f));
		g.fillEllipse(bandBounds[i]);
		g.setColour(COLOR_BACKGROUND());
		g.drawText(juce::String(i + 1), bandBounds[i], juce::Justification::centred);
	}

	// draw band curve button
	g.setColour(COLOR_KNOB_LABEL());
	UIUtils::drawBevel(g, bandBtn.getBounds().toFloat().expanded(1.f), 3.f, COLOR_BEVEL());
	auto mode = bandFilters[selband].mode;
	if (mode == SVF::PK) {
		UIUtils::drawPeak(g, bandBtn.getBounds().toFloat().translated(4.5f, 6.5f), COLOR_KNOB_LABEL());
	}
	else if (mode == SVF::LP) {
		UIUtils::drawLowpass(g, bandBtn.getBounds().toFloat().translated(4.5f, 6.5f), COLOR_KNOB_LABEL());
	}
	else if (mode == SVF::HP) {
		UIUtils::drawHighpass(g, bandBtn.getBounds().toFloat().translated(4.5f, 6.5f), COLOR_KNOB_LABEL());
	}
	else if (mode == SVF::LS) {
		UIUtils::drawLowShelf(g, bandBtn.getBounds().toFloat().translated(4.5f, 6.5f), COLOR_KNOB_LABEL());
	}
	else if (mode == SVF::HS) {
		UIUtils::drawHighShelf(g, bandBtn.getBounds().toFloat().translated(4.5f, 6.5f), COLOR_KNOB_LABEL());
	}

}

void FXEQ::resized()
{
	UIFX::resized();
	// viewBounds is paint-only, derived from the freqknob position.
	viewBounds = juce::Rectangle<float>(140.f, (float)getHeight())
		.reduced(0.f, 5.f)
		.reduced(0.f, 5.f)
		.withRightX((float)freqknobs[0]->getX() - 10.f)
		.withY(10.f);

	toggleUIComponents();
}

void FXEQ::toggleUIComponents()
{
	for (int i = 0; i < globals::EQ_BANDS; ++i) {
		freqknobs[i]->setVisible(selband == i);
		qknobs[i]->setVisible(selband == i);
		gainknobs[i]->setVisible(selband == i);
	}

	repaint();
}

void FXEQ::updateEQCurve()
{
	const int numPoints = (int)viewBounds.getWidth();
	const float minFreq = 20.0f;
	const float maxFreq = 20000.0f;

	magPoints.clear();

	auto firstBandMode = (int)editor.audioProcessor.params.getRawParameterValue("fx_eq_band1_mode")->load();
	auto lastBandMode = (int)editor.audioProcessor.params.getRawParameterValue("fx_eq_band" + juce::String(globals::EQ_BANDS) + "_mode")->load();

	for (int i = 0; i < numPoints; ++i)
	{
		float norm = (float)i / (numPoints - 1);
		float freq = minFreq * std::pow(maxFreq / minFreq, norm);

		float mag = 1.0f;

		for (int b = 0; b < globals::EQ_BANDS; ++b)
		{
			// FIX get modulated params only if the modulation voice is active
			// avoids constant animation of eq curve
			auto modulationActive = editor.audioProcessor.modulation->isAnyVoiceActive();

			auto pre = "fx_eq_band" + juce::String(b + 1);
			auto srate = editor.audioProcessor.srate / 2.f;
			auto cutoff = modulationActive
				? editor.audioProcessor.modulation->getValue(pre + "_freq")
				: editor.audioProcessor.params.getRawParameterValue(pre + "_freq")->load();
			auto gain = modulationActive
				? editor.audioProcessor.modulation->getValue(pre + "_gain")
				: editor.audioProcessor.params.getRawParameterValue(pre + "_gain")->load();
			gain = exp(gain * globals::DB2LOG);
			auto q = modulationActive
				? editor.audioProcessor.modulation->getValue(pre + "_q")
				: editor.audioProcessor.params.getRawParameterValue(pre + "_q")->load();
			SVF::Mode mode = b == 0 && firstBandMode == 0 ? SVF::HP
				: b == 0 && firstBandMode == 1 ? SVF::LS
				: b == globals::EQ_BANDS - 1 && lastBandMode == 0 ? SVF::LP
				: b == globals::EQ_BANDS - 1 && lastBandMode == 1 ? SVF::HS
				: SVF::PK;

			if (mode == SVF::LP) bandFilters[b].lp(srate, cutoff, q);
			else if (mode == SVF::LS) bandFilters[b].ls(srate, cutoff, q, gain);
			else if (mode == SVF::HP) bandFilters[b].hp(srate, cutoff, q);
			else if (mode == SVF::HS) bandFilters[b].hs(srate, cutoff, q, gain);
			else bandFilters[b].pk(srate, cutoff, q, gain);

			mag *= bandFilters[b].getMagnitude(freq);
		}

		magPoints.push_back((mag));
	}
}

void FXEQ::showBandModeMenu()
{
	auto mode = SVF::PK;

	if (selband == 0) {
		mode = (int)editor.audioProcessor.params.getRawParameterValue("fx_eq_band1_mode")->load() == 0
			? SVF::LP : SVF::LS;
	}
	if (selband == globals::EQ_BANDS - 1) {
		mode = (int)editor.audioProcessor.params.getRawParameterValue("fx_eq_band" + juce::String(globals::EQ_BANDS) + "_mode")->load() == 0
			? SVF::HP : SVF::HS;
	}

	juce::PopupMenu menu;
	if (selband == 0) {
		menu.addItem(1, "Low Cut", true, mode == SVF::HP);
		menu.addItem(2, "Low Shelf", true, mode == SVF::LS);
	}
	else if (selband == globals::EQ_BANDS - 1) {
		menu.addItem(3, "High Cut", true, mode == SVF::LP);
		menu.addItem(4, "High Shelf", true, mode == SVF::HS);
	}
	else {
		menu.addItem(5, "Peak", true, true);
	}

	auto menuPos = localPointToGlobal(bandBtn.getBounds().getBottomLeft());
	menu.setLookAndFeel(&getLookAndFeel());
	menu.showMenuAsync(juce::PopupMenu::Options()
		.withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
		.withTargetComponent(*this)
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this](int result) {
			if (result == 0) return;

            editor.audioProcessor.undomgr->createUndo();

			if (result == 1 || result == 2) {
				auto param = editor.audioProcessor.params.getParameter("fx_eq_band1_mode");
				param->setValueNotifyingHost(param->convertTo0to1((float)result - 1));
				toggleUIComponents();
			}
			if (result == 3 || result == 4) {
				auto param = editor.audioProcessor.params.getParameter("fx_eq_band" + juce::String(globals::EQ_BANDS) + "_mode");
				param->setValueNotifyingHost(param->convertTo0to1((float)result - 3));
				toggleUIComponents();
			}
		});
}
