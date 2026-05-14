#include "Meter.h"
#include "../../PluginProcessor.h"

float gainToScale(float g)
{
    return (std::log10(std::max(g, 0.001f)) + 3) / 4.f;
}

Meter::Meter(TetraOPAudioProcessor& p) : audioProcessor(p)
{
    zeroMeter = gainToScale(1.0f);

    startTimerHz(60);
}

void Meter::timerCallback()
{
    repaint();
}

Meter::~Meter()
{
}

void Meter::paint(juce::Graphics& g) {
	auto bounds = getLocalBounds().toFloat();
    auto barBounds = bounds.reduced(3.f).withHeight(5.f).translated(0.5f, 0.5f);
    float rawRmsL = audioProcessor.rmsL.load();
    float rawRmsR = audioProcessor.rmsR.load();
    constexpr float alpha = 0.15f;
    rmsSmoothedL = rmsSmoothedL < rawRmsL ? rawRmsL
        : (1.0f - alpha) * rmsSmoothedL + alpha * rawRmsL;
    rmsSmoothedR = rmsSmoothedR < rawRmsR ? rawRmsR
        : (1.0f - alpha) * rmsSmoothedR + alpha * rawRmsR;

    float rmsLeft = gainToScale(rmsSmoothedL);
    float rmsRight = gainToScale(rmsSmoothedR);

    g.setColour(COLOR_BEVEL());
    g.fillRoundedRectangle(bounds, 4.5f);

    juce::Path greenBars;
    greenBars.addRoundedRectangle(barBounds.getX(), barBounds.getY(), barBounds.getWidth(), barBounds.getHeight(), 1.5f);
    greenBars.addRoundedRectangle(barBounds.getX(), barBounds.getY() + barBounds.getHeight() + 4.f, barBounds.getWidth(), barBounds.getHeight(), 1.5f);
    g.saveState();
    //g.reduceClipRegion(greenBars);

    if (rmsLeft > 0.f) {
		g.setColour(COLOR_ACTIVE());
        auto lbar = barBounds
            .withWidth(barBounds.getWidth() * (float)std::min(zeroMeter, rmsLeft));
		g.fillRect(lbar);

		if (rmsLeft > zeroMeter) {
            lbar = barBounds
                .withX(barBounds.getX() + barBounds.getWidth() * zeroMeter)
                .withWidth(barBounds.getWidth() * (rmsLeft - zeroMeter));

			g.setColour(juce::Colours::red);
			g.fillRect(lbar);
		}
	}

    if (rmsRight > 0.f) {
		g.setColour(COLOR_ACTIVE());
		auto rbar = barBounds
			.withWidth(barBounds.getWidth() * (float)std::min(zeroMeter, rmsRight))
			.withY(barBounds.getY() + barBounds.getHeight() + 2);
        g.fillRect(rbar);

		if (rmsRight > zeroMeter) {
			g.setColour(juce::Colours::red);
			rbar = barBounds
				.withX(barBounds.getX() + barBounds.getWidth() * zeroMeter)
				.withWidth(barBounds.getWidth() * (rmsRight - zeroMeter))
				.withY(bounds.getBottom() - bounds.getHeight() / 4 - barBounds.getHeight() / 2 - 1.f);
			g.fillRect(rbar);
		}
	}
	g.restoreState();
}
