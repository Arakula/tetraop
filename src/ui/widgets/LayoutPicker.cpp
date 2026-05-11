#include "LayoutPicker.h"
#include "../../PluginEditor.h"

static void drawA_B_C_D(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx - r * 3, cy };
    Point<float> b = { cx - r, cy };
    Point<float> c = { cx + r, cy };
    Point<float> d = { cx + r * 3, cy };

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDCBA(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx, cy + r * 3 };
    Point<float> b = { cx, cy + r };
    Point<float> c = { cx, cy - r };
    Point<float> d = { cx, cy - r * 3 };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, c));
    g.setColour(COLOR_C());
    g.drawLine(Line<float>(c, b));
    g.setColour(COLOR_B());
    g.drawLine(Line<float>(b, a));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDC_BA(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx - r, cy + r };
    Point<float> b = { cx - r, cy - r };
    Point<float> c = { cx + r, cy + r };
    Point<float> d = { cx + r, cy - r };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, c));
    g.setColour(COLOR_B());
    g.drawLine(Line<float>(b, a));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDC_B_A(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx - r * 2, cy + r };
    Point<float> b = { cx, cy + r };
    Point<float> c = { cx + r * 2, cy + r };
    Point<float> d = { cx + r * 2, cy - r };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, c));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDA_DB_DC(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx - r * 2, cy + r };
    Point<float> b = { cx, cy + r };
    Point<float> c = { cx + r * 2, cy + r };
    Point<float> d = { cx, cy - r };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, b));
    g.drawLine(a.getX(), a.getY(), a.getX(), a.getY() - r);
    g.drawLine(c.getX(), c.getY(), c.getX(), c.getY() - r);
    g.drawLine(a.getX(), a.getY() - r, c.getX(), c.getY() - r);

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawBA_CA_DA(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx, cy + r };
    Point<float> b = { cx - r * 2, cy - r };
    Point<float> c = { cx, cy - r };
    Point<float> d = { cx + r * 2, cy - r };

    g.setColour(COLOR_C());
    g.drawLine(Line<float>(c, a));
    g.setColour(COLOR_B());
    g.drawLine(b.getX(), b.getY(), b.getX(), b.getY() + r);
    g.drawLine(b.getX() + r * 2, b.getY() + r, b.getX(), b.getY() + r);
    g.setColour(COLOR_D());
    g.drawLine(d.getX(), d.getY(), d.getX(), d.getY() + r);
    g.drawLine(d.getX() - r * 2, d.getY() + r, d.getX(), d.getY() + r);

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawA_CB_DC(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx - r, cy + r * 2 };
    Point<float> b = { cx + r, cy + r * 2 };
    Point<float> c = { cx + r, cy };
    Point<float> d = { cx + r, cy - r * 2 };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, c));
    g.setColour(COLOR_C());
    g.drawLine(Line<float>(c, b));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDC_CA_CB(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx - r, cy + r * 2 };
    Point<float> b = { cx + r, cy + r * 2 };
    Point<float> c = { cx, cy };
    Point<float> d = { cx, cy - r * 2 };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, c));
    g.setColour(COLOR_C());
    g.drawLine(Line<float>(c, a));
    g.drawLine(Line<float>(c, b));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDC_DB_BA_CA(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx, cy + r * 2 };
    Point<float> b = { cx - r, cy };
    Point<float> c = { cx + r, cy };
    Point<float> d = { cx, cy - r * 2 };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, c));
    g.drawLine(Line<float>(d, b));
    g.setColour(COLOR_C());
    g.drawLine(Line<float>(c, a));
    g.setColour(COLOR_B());
    g.drawLine(Line<float>(b, a));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDB_CB_BA(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx, cy + r * 2 };
    Point<float> b = { cx, cy };
    Point<float> c = { cx + r, cy - r * 2 };
    Point<float> d = { cx - r, cy - r * 2 };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, b));
    g.setColour(COLOR_C());
    g.drawLine(Line<float>(c, b));
    g.setColour(COLOR_B());
    g.drawLine(Line<float>(b, a));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

void LayoutPickerWidget::paint(Graphics& g)
{
    auto r = 40.f;
    if (hoverx > -1 && hovery > -1)
    {
        g.setColour(Colours::white.withAlpha(0.2f));
        if (hoverx > 4)
        {
            g.fillRect(40 * 5, 0, 40, 80);
        }
        else
        {
            g.fillRect(40 * hoverx, 40 * hovery, 40, 40);
        }
    }

    auto b = getLocalBounds().toFloat().withWidth(r).withHeight(r);
    drawA_B_C_D(g, b, 0.5f);
    drawDCBA(g, b.translated(r, 0), 0.5f);
    drawDC_BA(g, b.translated(r * 2, 0), 0.5f);
    drawDC_B_A(g, b.translated(r * 3, 0), 0.5f);
    drawDA_DB_DC(g, b.translated(r * 4, 0), 0.5f);
    drawBA_CA_DA(g, b.translated(0, r), 0.5f);
    drawA_CB_DC(g, b.translated(r, r), 0.5f);
    drawDC_CA_CB(g, b.translated(r * 2, r), 0.5f);
    drawDC_DB_BA_CA(g, b.translated(r * 3, r), 0.5f);
    drawDB_CB_BA(g, b.translated(r * 4, r), 0.5f);

    g.setFont(FontOptions(12.f));
    g.setColour(COLOR_KNOB_LABEL());
    auto bb = b.withHeight(16.f).translated(40 * 5.f, 3.f);
    g.drawText("C", bb, Justification::centred);
    g.drawText("U", bb.translated(0, 12.f), Justification::centred);
    g.drawText("S", bb.translated(0, 24.f), Justification::centred);
    g.drawText("T", bb.translated(0, 36.f), Justification::centred);
    g.drawText("O", bb.translated(0, 48.f), Justification::centred);
    g.drawText("M", bb.translated(0, 60.f), Justification::centred);
}

// =========================================================

void LayoutPickerWidget::mouseDown(const MouseEvent&)
{
    if (hoverx > -1 && hovery > -1 && onClick)
    {
        auto sqr = hoverx + hovery * 5;
        if (hoverx > 4) sqr = 11;
        onClick(sqr);
    }
}

void LayoutPickerWidget::mouseMove(const MouseEvent& e)
{
    hoverx = e.getPosition().x / 40;
    hovery = e.getPosition().y / 40;

    repaint();
}

void LayoutPickerWidget::mouseExit(const MouseEvent&)
{
    hoverx = -1;
    hovery = -1;
    repaint();
}

// ========================================================

LayoutPicker::LayoutPicker(TetraOPAudioProcessorEditor& p)
	: editor(p)
{
    editor.audioProcessor.params.addParameterListener("layout", this);

    addChildComponent(editBtn);
    editBtn.setAlpha(0.f);
    editBtn.onClick = [this] 
        { 
            editBtn.setVisible(false);
            editor.toggleFmMatrix(); 
        };
}

LayoutPicker::~LayoutPicker()
{
    editor.audioProcessor.params.removeParameterListener("layout", this);
}

void LayoutPicker::parameterChanged(const juce::String& pid, float val)
{
    if (pid == "layout" && val != 10.f)
        editBtn.setVisible(false);

	MessageManager::callAsync([this]{ repaint(); });
}


void LayoutPicker::mouseEnter(const MouseEvent& e)
{
    auto screenPos = e.getScreenPosition();
    if (!getScreenBounds().contains(screenPos))
        return; // ignore child components mouse enter

    int layout = (int)editor.audioProcessor.params.getRawParameterValue("layout")->load();
    if (layout == 10)
        editBtn.setVisible(true);
    repaint();
}

void LayoutPicker::mouseExit(const MouseEvent& e)
{
    auto screenPos = e.getScreenPosition();
    if (getScreenBounds().contains(screenPos))
        return; // ignore child components mouse exit

    editBtn.setVisible(false);
    repaint();
}

void LayoutPicker::mouseDown(const MouseEvent&)
{
    auto parentBounds = getScreenBounds();
    juce::Rectangle<int> targetArea(
        parentBounds.getX() + 30,
        parentBounds.getY() + 60,
        1,
        1
    );

    auto* content = new LayoutPickerWidget();
    content->setSize(40 * 6, 40 * 2);
    auto& callout = juce::CallOutBox::launchAsynchronously(
        std::unique_ptr<juce::Component>(content),
        targetArea,
        nullptr
    );

    content->onClick = [&callout, this](int res)
        {
            auto param = editor.audioProcessor.params.getParameter("layout");
            param->setValueNotifyingHost(param->convertTo0to1((float)res));
            if (res == 11) editor.toggleFmMatrix();
            callout.exitModalState(0);
        };
}

void LayoutPicker::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    (void)event;
    (void)wheel;
}

void LayoutPicker::resized()
{
    auto b = getLocalBounds();
    editBtn.setBounds(Rectangle<int>(40, 20).withX(b.getCentreX() - 20).withY(5));
}

void LayoutPicker::paint(Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(COLOR_BACKGROUND().darker(0.5f));
    g.fillRoundedRectangle(b.reduced(0.5f), 5.f);

    if (editBtn.isVisible())
    {
        g.setColour(COLOR_KNOB_LABEL());
        g.fillRoundedRectangle(editBtn.getBounds().toFloat(), 3.f);
        g.setColour(COLOR_BACKGROUND());
        g.setFont(16.f);
        g.drawText("Edit", editBtn.getBounds().toFloat(), Justification::centred);
    }

    auto layout = (FmMatrix::Layout)editor.audioProcessor.params.getRawParameterValue("layout")->load();

    switch (layout)
    {
        case FmMatrix::A_B_C_D: 
            drawA_B_C_D(g, b, .75f);
            break;
        case FmMatrix::DCBA: 
            drawDCBA(g, b, 0.75f);
            break; 
        case FmMatrix::DC_BA: 
            drawDC_BA(g, b, 0.75f);
            break; 
        case FmMatrix::DC_B_A: 
            drawDC_B_A(g, b, 0.75f);
            break;
        case FmMatrix::DA_DB_DC: 
            drawDA_DB_DC(g, b, 0.75f);
            break;
        case FmMatrix::BA_CA_DA: 
            drawBA_CA_DA(g, b, 0.75f);
            break;
        case FmMatrix::A_CB_DC: 
            drawA_CB_DC(g, b, 0.75f);
            break;
        case FmMatrix::DC_CA_CB: 
            drawDC_CA_CB(g, b, 0.75f);
            break;
        case FmMatrix::DC_DB_BA_CA: 
            drawDC_DB_BA_CA(g, b, 0.75f);
            break;
        case FmMatrix::DB_CB_BA: 
            drawDB_CB_BA(g, b, 0.75f);
            break;
        default:
            g.setFont(FontOptions(16.f));
            g.setColour(COLOR_KNOB_LABEL());
            g.drawText("Custom", b, Justification::centred);
    }
}

