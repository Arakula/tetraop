#include "CustomLookAndFeel.h"

CustomLookAndFeel::CustomLookAndFeel()
{
    setColour(TooltipWindow::backgroundColourId, Colours::black.withAlpha(0.85f));
    setColour(TooltipWindow::textColourId, Colours::white);
    setColour(TooltipWindow::outlineColourId, Colours::transparentBlack);
    setColour(ScrollBar::thumbColourId, Colours::white);

    defaultFont = Typeface::createSystemTypefaceFor(BinaryData::PlayRegular_ttf, BinaryData::PlayRegular_ttfSize);
    defaultFontBold = Typeface::createSystemTypefaceFor(BinaryData::PlayBold_ttf, BinaryData::PlayBold_ttfSize);
    setDefaultSansSerifTypeface(defaultFont);
    this->setDefaultLookAndFeel(this);
}

Typeface::Ptr CustomLookAndFeel::getTypefaceForFont(const Font& font)
{
    (void)font;
    return defaultFont;
}

juce::Font CustomLookAndFeel::getBoldFont(float size)
{
    if (defaultFontBold != nullptr)
        return juce::Font(FontOptions(defaultFontBold)).withHeight(size);
    return juce::Font(juce::FontOptions().withHeight(size).withStyle("Bold"));
}

void CustomLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height)
{
    const int padding = 10;
    juce::Rectangle<float> area(0, 0, (float)width, (float)height);

    g.setColour(findColour(juce::TooltipWindow::backgroundColourId));
    g.fillRoundedRectangle(area, 4.0f);

    juce::AttributedString s;
    s.setJustification(juce::Justification::centredLeft);
    s.append(text, juce::Font(FontOptions(15.f)),
        findColour(juce::TooltipWindow::textColourId));

    juce::TextLayout tl;
    tl.createLayout(s, (float)width - 2 * padding);
    tl.draw(g, area.reduced((float)padding));
}

Rectangle<int> CustomLookAndFeel::getTooltipBounds(const String& tipText,
    Point<int> screenPos,
    Rectangle<int> parentArea)
{
    const juce::Font font(FontOptions(15.f));
    const int maxWidth = 400;
    const int padding = 10;

    // Create layout to measure text
    juce::AttributedString s;
    s.setJustification(juce::Justification::centredLeft);
    s.append(tipText, font, findColour(juce::TooltipWindow::textColourId));

    juce::TextLayout tl;
    tl.createLayout(s, (float)maxWidth);

    // Calculate total layout bounds manually
    float layoutWidth = 0.0f;
    float layoutHeight = 0.0f;

    for (int i = 0; i < tl.getNumLines(); ++i)
    {
        auto& line = tl.getLine(i);
        layoutWidth = std::max(layoutWidth, line.getLineBounds().getWidth());
        layoutHeight += line.getLineBounds().getHeight();
    }

    const int totalWidth = (int)std::ceil(layoutWidth) + padding * 2;
    const int totalHeight = (int)std::ceil(layoutHeight) + padding * 2;

    // Position tooltip near mouse
    juce::Rectangle<int> r(screenPos.x, screenPos.y, totalWidth, totalHeight);

    // Ensure it's within the visible area manually
    if (r.getRight() > parentArea.getRight())
        r.setX(parentArea.getRight() - totalWidth);
    if (r.getBottom() > parentArea.getBottom())
        r.setY(parentArea.getBottom() - totalHeight);
    if (r.getX() < parentArea.getX())
        r.setX(parentArea.getX());
    if (r.getY() < parentArea.getY())
        r.setY(parentArea.getY());

    return r;
}