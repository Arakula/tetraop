#pragma once

#include <JuceHeader.h>
#include "../UIUtils.h"

/*
* Draws a curved cable (catenary) from a mod source to the mouse position.
* Modulatable components themselves draw the overlay color.
*/
class DragDropOverlay : public juce::Component
{
public:
	juce::Colour arrowColor{};
	juce::Point<float> arrowStart{};
	juce::Point<float> arrowEnd{};

	DragDropOverlay() {}
	~DragDropOverlay() override {}

	void paint(juce::Graphics& g) override
	{
		arrowEnd = getMouseXYRelative().toFloat();

		auto x0 = arrowStart.x;
		auto y0 = arrowStart.y;
		auto x1 = arrowEnd.x;
		auto y1 = arrowEnd.y;

		// Catenary requires distinct x values
		if (std::abs(x1 - x0) < 1.0f)
			x1 = x0 + 1.0f;

		// More slack the further apart the endpoints are
		auto distance = juce::Point<float>(x0, y0).getDistanceFrom({ x1, y1 });
		auto slack = std::max(20.0f, distance * 0.3f);

		juce::Path cable;
		try
		{
			gin::Catenary catenary(x0, y0, x1, y1, slack, 3, true);

			auto startX = std::min(x0, x1);
			auto endX = std::max(x0, x1);
			auto step = std::max(1.0f, (endX - startX) / 60.0f);

			cable.startNewSubPath(startX, catenary.calcY(startX));
			for (auto x = startX + step; x < endX; x += step)
				cable.lineTo(x, catenary.calcY(x));
			cable.lineTo(endX, catenary.calcY(endX));
		}
		catch (...)
		{
			cable.startNewSubPath(arrowStart);
			cable.lineTo(arrowEnd);
		}

		// Outline / shadow under the cable
		g.setColour(juce::Colours::black.withAlpha(0.5f));
		g.strokePath(cable, juce::PathStrokeType(lineThickness + outlineThickness * 2.0f,
			juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

		// Cable
		g.setColour(arrowColor);
		g.strokePath(cable, juce::PathStrokeType(lineThickness,
			juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

		// Endpoint dots
		auto drawDot = [&](juce::Point<float> p)
		{
			auto r = dotRadius + outlineThickness;
			g.setColour(juce::Colours::black.withAlpha(0.5f));
			g.fillEllipse(p.x - r, p.y - r, r * 2.0f, r * 2.0f);
			g.setColour(arrowColor);
			g.fillEllipse(p.x - dotRadius, p.y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
		};
		drawDot(arrowStart);
		drawDot(arrowEnd);
	}

private:
	static constexpr float dotRadius = 4.0f;
	static constexpr float lineThickness = 2.5f;
	static constexpr float outlineThickness = 1.0f;
};
