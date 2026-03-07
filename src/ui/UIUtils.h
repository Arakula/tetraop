#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class RipplerAudioProcessorEditor;

class UIUtils {
public:
	static void drawBevel(Graphics& g, Rectangle<float> bounds, float corner, Colour bg);
	static void drawTriangle(Graphics& g, Rectangle<float> bounds, int direction, Colour c);
	static void drawPanel(Graphics& g, Rectangle<float> bounds, bool drawHeader);
	static void drawVSeparator(Graphics& g, Rectangle<float> bounds);
	static void drawCheckmark(Graphics& g, Rectangle<float> bounds, Colour bg, Colour check, bool checked);
};