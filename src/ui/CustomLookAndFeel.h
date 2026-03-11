#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "UIUtils.h"

using namespace globals;

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    float scale = 1.f;

    CustomLookAndFeel();
    Typeface::Ptr getTypefaceForFont(const juce::Font&) override;
    juce::Font getBoldFont(float size);
    int getPopupMenuBorderSize() override { return 5; };

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        (void)width;
        (void)height;
        g.setColour(Colours::black);
        g.fillAll();
    }

    void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override;

    Rectangle<int> getTooltipBounds(const String& tipText,
        Point<int> screenPos,
        Rectangle<int> parentArea
    ) override;

    Font getPopupMenuFont() override
    {
        return juce::Font(FontOptions(16.f));
    }


private:
    juce::Typeface::Ptr defaultFont;
    juce::Typeface::Ptr defaultFontBold;
};