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
        siteLink.setURL(URL("https://github.com/tiagolr/rippler"));
        siteLink.setButtonText("github.com");
        siteLink.setColour(HyperlinkButton::ColourIds::textColourId, COLOR_ACTIVE());
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
        //UIUtils::drawTilt(g, row.toFloat().translated(190.f, 0.f), Colours::white, 1.f);

        row = panel.removeFromTop(70);

        g.saveState();
        //UIUtils::drawLogo(g, row.toFloat().withTrimmedLeft(100.f).translated(20.f, 20.f), COLOR_TEXT_BRIGHT(), 1.5f);
        g.setColour(Colours::white);
        g.drawText("RIPPLER", row, Justification::centred);
        g.drawText(String("v") + String(PROJECT_VERSION), row.withTrimmedTop(50), Justification::centred);
        g.restoreState();

        g.setFont(FontOptions(16.0f));
        row = panel.removeFromTop(22);
        siteLink.setBounds(row.reduced(150, 0));

        panel.removeFromTop(20);
        row = panel.removeFromTop(20);
        g.setColour(Colours::white.withAlpha(0.5f));
        g.drawText("Coding", Rectangle<int>(150, 20).withX(row.getCentreX() - 150 - 75).withY(row.getY()), Justification::centred);
        g.setColour(Colours::white);
        g.drawText("Tilr", Rectangle<int>(150, 20).withX(row.getCentreX() - 150 - 75).withY(row.getY() + 20), Justification::centred);

        g.setColour(Colours::white.withAlpha(0.5f));
        g.drawText("Design", Rectangle<int>(150, 20).withX(row.getCentreX() - 75).withY(row.getY()), Justification::centred);
        g.setColour(Colours::white);
        g.drawText("A.Jasinski", Rectangle<int>(150, 20).withX(row.getCentreX() - 75).withY(row.getY() + 20), Justification::centred);

        g.setColour(Colours::white.withAlpha(0.5f));
        g.drawText("Reverb", Rectangle<int>(150, 20).withX(row.getCentreX() + 75).withY(row.getY()), Justification::centred);
        g.setColour(Colours::white);
        g.drawText("Taron", Rectangle<int>(150, 20).withX(row.getCentreX() + 75).withY(row.getY() + 20), Justification::centred);

        row = panel.removeFromTop(100).translated(0, 50);
        g.setColour(Colours::white.withAlpha(0.5f));
        g.drawText("Special thanks to all involved in dev, beta testing and factory library:",
            row.removeFromTop(20), Justification::centred);
        g.setColour(Colours::white.withAlpha(1.f));
        g.drawText("Andreya A., AU33, ElVicente, fbr, JaSh, Kangal8or, KlashKontakt", row.removeFromTop(20).withTrimmedLeft(45), Justification::centredLeft);
        g.drawText("nbiar, RichardSofOSC, Starsickle, RichardSofOSC, Starsickle", row.removeFromTop(20).withTrimmedLeft(45), Justification::centredLeft);
        g.drawText("The Sound of Merlin, Tobias Miller, IV, z.prime", row.removeFromTop(20).withTrimmedLeft(45), Justification::centredLeft);

    }
};