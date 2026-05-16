#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"

using namespace globals;

class AboutDialog : public juce::Component
{
public:
    HyperlinkButton siteLink;

    AboutDialog()
    {
        addAndMakeVisible(siteLink);
        siteLink.setURL(URL("https://github.com/tiagolr/tetraop"));
        siteLink.setButtonText("github.com");
        siteLink.setColour(HyperlinkButton::ColourIds::textColourId, COLOR_ACTIVE());
        siteLink.setFont(FontOptions(22.f), false);
    }
    ~AboutDialog() override {}

    void mouseDown(const MouseEvent& e) override
    {
        (void)e;
        setVisible(false);
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds();
        int w = 600;
        int h = 500;

        g.fillAll(Colours::black.withAlpha(0.5f));

        auto panel = Rectangle<int>( bounds.getCentreX() - w / 2, bounds.getCentreY() - h / 2, w, h );

        g.setColour(Colours::black.withAlpha(0.9f));
        auto ds = DropShadow(Colours::black, 30, { 0, 0 });
        ds.drawForRectangle(g, panel);
        g.fillRect(panel);

        panel.reduce(40, 20);
        g.setColour(Colours::white);
        auto row = panel.removeFromTop(25);

        row = panel.removeFromTop(70);

        g.saveState();
        //UIUtils::drawLogo(g, row.toFloat().withTrimmedLeft(100.f).translated(20.f, 20.f), COLOR_TEXT_BRIGHT(), 1.5f);
        g.setColour(Colours::white);
        g.setFont(FontOptions(22.f));
        g.drawText("TetraOP", row, Justification::centred);
        g.drawText(String("v") + String(PROJECT_VERSION), row.withTrimmedTop(50), Justification::centred);
        g.restoreState();

        g.setFont(FontOptions(18.0f));
        row = panel.removeFromTop(32);
        siteLink.setBounds(row.reduced(150, 0));

        panel.removeFromTop(20);
        row = panel.removeFromTop(20);
        g.setColour(Colours::white.withAlpha(0.5f));

        g.setColour(Colours::white.withAlpha(0.5f));
        g.drawText("By", Rectangle<int>(150, 20).withX(row.getCentreX() - 75).withY(row.getY()), Justification::centred);
        g.setColour(Colours::white);
        g.drawText("Tilr", Rectangle<int>(150, 20).withX(row.getCentreX() - 75).withY(row.getY() + 20), Justification::centred);

        row = panel.removeFromTop(100).translated(0, 50);
        g.setColour(Colours::white);
        g.drawText("[Shift] for fine adjustments",
            row.removeFromTop(20), Justification::centred);
        g.drawText("[Ctrl] for quick modulation connection",
            row.removeFromTop(20), Justification::centred);
        g.drawText("[Alt] for LFO multi-selection skewing",
            row.removeFromTop(20), Justification::centred);
    }
};