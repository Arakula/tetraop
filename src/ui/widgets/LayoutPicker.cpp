#include "LayoutPicker.h"
#include "../../PluginEditor.h"

static void drawSquares(Graphics& g, Rectangle<float> bounds, float scale, int enabledMask)
{
    float h = 10 * std::sqrt(scale);
    float pad = 10 * scale;

    auto a = Rectangle<float>(bounds.getX() + pad, bounds.getY() + pad, h, h);
    auto b = Rectangle<float>(bounds.getRight() - pad - h, bounds.getY() + pad, h, h);
    auto c = Rectangle<float>(bounds.getX() + pad, bounds.getBottom() - pad - h, h, h);
    auto d = Rectangle<float>(bounds.getRight() - pad - h, bounds.getBottom() - pad - h, h, h);

    bool aon = (enabledMask & (1 << 3)) != 0;
    bool bon = (enabledMask & (1 << 2)) != 0;
    bool con = (enabledMask & (1 << 1)) != 0;
    bool don = (enabledMask & (1 << 0)) != 0;

    g.setColour(COLOR_BACKGROUND().darker(0.5f));
    g.fillRect(a.expanded(4.f));
    g.fillRect(b.expanded(4.f));
    g.fillRect(c.expanded(4.f));
    g.fillRect(d.expanded(4.f));

    g.setColour(COLOR_A());
    if (aon) g.fillRect(a); else g.drawRect(a, 1.4f);
    g.setColour(COLOR_B());
    if (bon) g.fillRect(b); else g.drawRect(b, 1.4f);
    g.setColour(COLOR_C());
    if (con) g.fillRect(c); else g.drawRect(c, 1.4f);
    g.setColour(COLOR_D());
    if (don) g.fillRect(d); else g.drawRect(d, 1.4f);

}

static void drawConnection(Graphics& g, Rectangle<float> bounds, float scale, int start, int end)
{
    float h = 10 * std::sqrt(scale);
    float pad = 10 * scale;

    auto a = Rectangle<float>(bounds.getX() + pad, bounds.getY() + pad, h, h);
    auto b = Rectangle<float>(bounds.getRight() - pad - h, bounds.getY() + pad, h, h);
    auto c = Rectangle<float>(bounds.getX() + pad, bounds.getBottom() - pad - h, h, h);
    auto d = Rectangle<float>(bounds.getRight() - pad - h, bounds.getBottom() - pad - h, h, h);

    Rectangle<float> sqrs[] = { a, b, c, d};
    g.setColour(start == 0 ? COLOR_A() : start == 1 ? COLOR_B() : start == 2 ? COLOR_C() : COLOR_D());
    g.drawLine({ sqrs[start].getCentre(), sqrs[end].getCentre() }, 1.4f);
}

static void drawA_B_C_D(Graphics& g, Rectangle<float> bounds, float scale)
{
    drawSquares(g, bounds, scale, 0b1111);
}

static void drawDCBA(Graphics& g, Rectangle<float> bounds, float scale)
{
    drawConnection(g, bounds, scale, 3, 2);
    drawConnection(g, bounds, scale, 2, 1);
    drawConnection(g, bounds, scale, 1, 0);
    drawSquares(g, bounds, scale, 0b1000);
}

static void drawCA_DB(Graphics& g, Rectangle<float> bounds, float scale)
{
    drawConnection(g, bounds, scale, 2, 0);
    drawConnection(g, bounds, scale, 3, 1);
    drawSquares(g, bounds, scale, 0b1100);
}

static void drawDB_C_A(Graphics& g, Rectangle<float> bounds, float scale)
{
    drawConnection(g, bounds, scale, 3, 1);
    drawSquares(g, bounds, scale, 0b1110);
}

static void drawDA_DB_DC(Graphics& g, Rectangle<float> bounds, float scale)
{
    drawConnection(g, bounds, scale, 3, 0);
    drawConnection(g, bounds, scale, 3, 1);
    drawConnection(g, bounds, scale, 3, 2);
    drawSquares(g, bounds, scale, 0b1110);
}

static void drawBA_CA_DA(Graphics& g, Rectangle<float> bounds, float scale)
{
    drawConnection(g, bounds, scale, 1, 0);
    drawConnection(g, bounds, scale, 2, 0);
    drawConnection(g, bounds, scale, 3, 0);
    drawSquares(g, bounds, scale, 0b1000);
}

static void drawA_CB_DC(Graphics& g, Rectangle<float> bounds, float scale)
{
    drawConnection(g, bounds, scale, 2, 1);
    drawConnection(g, bounds, scale, 3, 2);
    drawSquares(g, bounds, scale, 0b1100);
}

static void drawDC_CA_CB(Graphics& g, Rectangle<float> bounds, float scale)
{
    drawConnection(g, bounds, scale, 3, 2);
    drawConnection(g, bounds, scale, 2, 0);
    drawConnection(g, bounds, scale, 2, 1);
    drawSquares(g, bounds, scale, 0b1100);
}

static void drawDC_DB_BA_CA(Graphics& g, Rectangle<float> bounds, float scale)
{
    drawConnection(g, bounds, scale, 3, 2);
    drawConnection(g, bounds, scale, 3, 1);
    drawConnection(g, bounds, scale, 1, 0);
    drawConnection(g, bounds, scale, 2, 0);
    drawSquares(g, bounds, scale, 0b1000);
}

static void drawDB_CB_BA(Graphics& g, Rectangle<float> bounds, float scale)
{
    drawConnection(g, bounds, scale, 3, 1);
    drawConnection(g, bounds, scale, 2, 1);
    drawConnection(g, bounds, scale, 1, 0);
    drawSquares(g, bounds, scale, 0b1000);
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
    drawCA_DB(g, b.translated(r * 2, 0), 0.5f);
    drawDB_C_A(g, b.translated(r * 3, 0), 0.5f);
    drawDA_DB_DC(g, b.translated(r * 4, 0), 0.5f);
    drawBA_CA_DA(g, b.translated(0, r), 0.5f);
    drawA_CB_DC(g, b.translated(r, r), 0.5f);
    drawDC_CA_CB(g, b.translated(r * 2, r), 0.5f);
    drawDC_DB_BA_CA(g, b.translated(r * 3, r), 0.5f);
    drawDB_CB_BA(g, b.translated(r * 4, r), 0.5f);

    g.setFont(FontOptions(12.f));
    g.setColour(COLOR_KNOB_LABEL());
    auto bb = b.withHeight(16.f).translated(40 * 5.f, 3.f);
    g.drawText("M", bb, Justification::centred);
    g.drawText("A", bb.translated(0, 12.f), Justification::centred);
    g.drawText("T", bb.translated(0, 24.f), Justification::centred);
    g.drawText("R", bb.translated(0, 36.f), Justification::centred);
    g.drawText("I", bb.translated(0, 48.f), Justification::centred);
    g.drawText("X", bb.translated(0, 60.f), Justification::centred);
}

// =========================================================

void LayoutPickerWidget::mouseUp(const MouseEvent&)
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

    editor.audioProcessor.params.addParameterListener("a_feedback", this);
    editor.audioProcessor.params.addParameterListener("fm_ab", this);
    editor.audioProcessor.params.addParameterListener("fm_ac", this);
    editor.audioProcessor.params.addParameterListener("fm_ad", this);
    editor.audioProcessor.params.addParameterListener("fm_ba", this);
    editor.audioProcessor.params.addParameterListener("b_feedback", this);
    editor.audioProcessor.params.addParameterListener("fm_bc", this);
    editor.audioProcessor.params.addParameterListener("fm_bd", this);
    editor.audioProcessor.params.addParameterListener("fm_ca", this);
    editor.audioProcessor.params.addParameterListener("fm_cb", this);
    editor.audioProcessor.params.addParameterListener("c_feedback", this);
    editor.audioProcessor.params.addParameterListener("fm_cd", this);
    editor.audioProcessor.params.addParameterListener("fm_da", this);
    editor.audioProcessor.params.addParameterListener("fm_db", this);
    editor.audioProcessor.params.addParameterListener("fm_dc", this);
    editor.audioProcessor.params.addParameterListener("d_feedback", this);

    editor.audioProcessor.params.addParameterListener("fm_aout", this);
    editor.audioProcessor.params.addParameterListener("fm_bout", this);
    editor.audioProcessor.params.addParameterListener("fm_cout", this);
    editor.audioProcessor.params.addParameterListener("fm_dout", this);

    editor.audioProcessor.params.addParameterListener("rm_aa", this);
    editor.audioProcessor.params.addParameterListener("rm_ab", this);
    editor.audioProcessor.params.addParameterListener("rm_ac", this);
    editor.audioProcessor.params.addParameterListener("rm_ad", this);
    editor.audioProcessor.params.addParameterListener("rm_ba", this);
    editor.audioProcessor.params.addParameterListener("rm_bb", this);
    editor.audioProcessor.params.addParameterListener("rm_bc", this);
    editor.audioProcessor.params.addParameterListener("rm_bd", this);
    editor.audioProcessor.params.addParameterListener("rm_ca", this);
    editor.audioProcessor.params.addParameterListener("rm_cb", this);
    editor.audioProcessor.params.addParameterListener("rm_cc", this);
    editor.audioProcessor.params.addParameterListener("rm_cd", this);
    editor.audioProcessor.params.addParameterListener("rm_da", this);
    editor.audioProcessor.params.addParameterListener("rm_db", this);
    editor.audioProcessor.params.addParameterListener("rm_dc", this);
    editor.audioProcessor.params.addParameterListener("rm_dd", this);
}

LayoutPicker::~LayoutPicker()
{
    editor.audioProcessor.params.removeParameterListener("layout", this);

    editor.audioProcessor.params.removeParameterListener("a_feedback", this);
    editor.audioProcessor.params.removeParameterListener("fm_ab", this);
    editor.audioProcessor.params.removeParameterListener("fm_ac", this);
    editor.audioProcessor.params.removeParameterListener("fm_ad", this);
    editor.audioProcessor.params.removeParameterListener("fm_ba", this);
    editor.audioProcessor.params.removeParameterListener("b_feedback", this);
    editor.audioProcessor.params.removeParameterListener("fm_bc", this);
    editor.audioProcessor.params.removeParameterListener("fm_bd", this);
    editor.audioProcessor.params.removeParameterListener("fm_ca", this);
    editor.audioProcessor.params.removeParameterListener("fm_cb", this);
    editor.audioProcessor.params.removeParameterListener("c_feedback", this);
    editor.audioProcessor.params.removeParameterListener("fm_cd", this);
    editor.audioProcessor.params.removeParameterListener("fm_da", this);
    editor.audioProcessor.params.removeParameterListener("fm_db", this);
    editor.audioProcessor.params.removeParameterListener("fm_dc", this);
    editor.audioProcessor.params.removeParameterListener("d_feedback", this);

    editor.audioProcessor.params.removeParameterListener("fm_aout", this);
    editor.audioProcessor.params.removeParameterListener("fm_bout", this);
    editor.audioProcessor.params.removeParameterListener("fm_cout", this);
    editor.audioProcessor.params.removeParameterListener("fm_dout", this);

    editor.audioProcessor.params.removeParameterListener("rm_aa", this);
    editor.audioProcessor.params.removeParameterListener("rm_ab", this);
    editor.audioProcessor.params.removeParameterListener("rm_ac", this);
    editor.audioProcessor.params.removeParameterListener("rm_ad", this);
    editor.audioProcessor.params.removeParameterListener("rm_ba", this);
    editor.audioProcessor.params.removeParameterListener("rm_bb", this);
    editor.audioProcessor.params.removeParameterListener("rm_bc", this);
    editor.audioProcessor.params.removeParameterListener("rm_bd", this);
    editor.audioProcessor.params.removeParameterListener("rm_ca", this);
    editor.audioProcessor.params.removeParameterListener("rm_cb", this);
    editor.audioProcessor.params.removeParameterListener("rm_cc", this);
    editor.audioProcessor.params.removeParameterListener("rm_cd", this);
    editor.audioProcessor.params.removeParameterListener("rm_da", this);
    editor.audioProcessor.params.removeParameterListener("rm_db", this);
    editor.audioProcessor.params.removeParameterListener("rm_dc", this);
    editor.audioProcessor.params.removeParameterListener("rm_dd", this);
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

    auto layout = (FmMatrix::Layout)editor.audioProcessor.params.getRawParameterValue("layout")->load();

    switch (layout)
    {
        case FmMatrix::A_B_C_D: 
            drawA_B_C_D(g, b, .75f);
            break;
        case FmMatrix::DCBA: 
            drawDCBA(g, b, 0.75f);
            break; 
        case FmMatrix::CA_DB: 
            drawCA_DB(g, b, 0.75f);
            break; 
        case FmMatrix::DB_C_A: 
            drawDB_C_A(g, b, 0.75f);
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
            drawCustomLayoutPreview(g, b);
    }

    if (editBtn.isVisible())
    {
        g.setColour(COLOR_KNOB_LABEL());
        g.fillRoundedRectangle(editBtn.getBounds().toFloat(), 3.f);
        g.setColour(COLOR_BACKGROUND());
        g.setFont(16.f);
        g.drawText("Edit", editBtn.getBounds().toFloat(), Justification::centred);
    }
}

void LayoutPicker::drawCustomLayoutPreview(Graphics& g, Rectangle<float> bounds)
{
    float fm_aa = editor.audioProcessor.params.getRawParameterValue("a_feedback")->load();
    float fm_ab = editor.audioProcessor.params.getRawParameterValue("fm_ab")->load();
    float fm_ac = editor.audioProcessor.params.getRawParameterValue("fm_ac")->load();
    float fm_ad = editor.audioProcessor.params.getRawParameterValue("fm_ad")->load();
    float fm_ba = editor.audioProcessor.params.getRawParameterValue("fm_ba")->load();
    float fm_bb = editor.audioProcessor.params.getRawParameterValue("b_feedback")->load();
    float fm_bc = editor.audioProcessor.params.getRawParameterValue("fm_bc")->load();
    float fm_bd = editor.audioProcessor.params.getRawParameterValue("fm_bd")->load();
    float fm_ca = editor.audioProcessor.params.getRawParameterValue("fm_ca")->load();
    float fm_cb = editor.audioProcessor.params.getRawParameterValue("fm_cb")->load();
    float fm_cc = editor.audioProcessor.params.getRawParameterValue("c_feedback")->load();
    float fm_cd = editor.audioProcessor.params.getRawParameterValue("fm_cd")->load();
    float fm_da = editor.audioProcessor.params.getRawParameterValue("fm_da")->load();
    float fm_db = editor.audioProcessor.params.getRawParameterValue("fm_db")->load();
    float fm_dc = editor.audioProcessor.params.getRawParameterValue("fm_dc")->load();
    float fm_dd = editor.audioProcessor.params.getRawParameterValue("d_feedback")->load();

    float rm_aa = editor.audioProcessor.params.getRawParameterValue("rm_aa")->load(); 
    float rm_ab = editor.audioProcessor.params.getRawParameterValue("rm_ab")->load();
    float rm_ac = editor.audioProcessor.params.getRawParameterValue("rm_ac")->load();
    float rm_ad = editor.audioProcessor.params.getRawParameterValue("rm_ad")->load();
    float rm_ba = editor.audioProcessor.params.getRawParameterValue("rm_ba")->load();
    float rm_bb = editor.audioProcessor.params.getRawParameterValue("rm_bb")->load();
    float rm_bc = editor.audioProcessor.params.getRawParameterValue("rm_bc")->load();
    float rm_bd = editor.audioProcessor.params.getRawParameterValue("rm_bd")->load();
    float rm_ca = editor.audioProcessor.params.getRawParameterValue("rm_ca")->load();
    float rm_cb = editor.audioProcessor.params.getRawParameterValue("rm_cb")->load();
    float rm_cc = editor.audioProcessor.params.getRawParameterValue("rm_cc")->load();
    float rm_cd = editor.audioProcessor.params.getRawParameterValue("rm_cd")->load();
    float rm_da = editor.audioProcessor.params.getRawParameterValue("rm_da")->load();
    float rm_db = editor.audioProcessor.params.getRawParameterValue("rm_db")->load();
    float rm_dc = editor.audioProcessor.params.getRawParameterValue("rm_dc")->load();
    float rm_dd = editor.audioProcessor.params.getRawParameterValue("rm_dd")->load();

    float w = bounds.getWidth();
    float h = bounds.getHeight();
    float r = 8.f;

    float gapX = (w - (4 * r)) / 5;
    float gapY = (h - (4 * r)) / 5;

    auto drawPie = [&](float norm, int x, int y, Colour c, bool isRm)
        {
            auto bb = Rectangle<float>(r, r)
                .withX(gapX + x * (r + gapX))
                .withY(gapY + y * (r + gapY));

            g.setColour(c.withAlpha(isRm ? 0.4f : 0.75f));
            Path p;
            p.addPieSegment(bb, 0, MathConstants<float>::twoPi * norm, 0.f);
            g.fillPath(p);
            g.setColour(c);
            g.drawEllipse(bb, 1.f);
        };

    drawPie(rm_aa, 0, 0, COLOR_A(), true);
    drawPie(rm_ab, 0, 1, COLOR_A(), true);
    drawPie(rm_ac, 0, 2, COLOR_A(), true);
    drawPie(rm_ad, 0, 3, COLOR_A(), true);

    drawPie(rm_ba, 1, 0, COLOR_B(), true);
    drawPie(rm_bb, 1, 1, COLOR_B(), true);
    drawPie(rm_bc, 1, 2, COLOR_B(), true);
    drawPie(rm_bd, 1, 3, COLOR_B(), true);

    drawPie(rm_ca, 2, 0, COLOR_C(), true);
    drawPie(rm_cb, 2, 1, COLOR_C(), true);
    drawPie(rm_cc, 2, 2, COLOR_C(), true);
    drawPie(rm_cd, 2, 3, COLOR_C(), true);

    drawPie(rm_da, 3, 0, COLOR_D(), true);
    drawPie(rm_db, 3, 1, COLOR_D(), true);
    drawPie(rm_dc, 3, 2, COLOR_D(), true);
    drawPie(rm_dd, 3, 3, COLOR_D(), true);

    //

    drawPie(fm_aa, 0, 0, COLOR_A(), false);
    drawPie(fm_ab, 0, 1, COLOR_A(), false);
    drawPie(fm_ac, 0, 2, COLOR_A(), false);
    drawPie(fm_ad, 0, 3, COLOR_A(), false);

    drawPie(fm_ba, 1, 0, COLOR_B(), false);
    drawPie(fm_bb, 1, 1, COLOR_B(), false);
    drawPie(fm_bc, 1, 2, COLOR_B(), false);
    drawPie(fm_bd, 1, 3, COLOR_B(), false);

    drawPie(fm_ca, 2, 0, COLOR_C(), false);
    drawPie(fm_cb, 2, 1, COLOR_C(), false);
    drawPie(fm_cc, 2, 2, COLOR_C(), false);
    drawPie(fm_cd, 2, 3, COLOR_C(), false);

    drawPie(fm_da, 3, 0, COLOR_D(), false);
    drawPie(fm_db, 3, 1, COLOR_D(), false);
    drawPie(fm_dc, 3, 2, COLOR_D(), false);
    drawPie(fm_dd, 3, 3, COLOR_D(), false);

    float fm_aout = editor.audioProcessor.params.getRawParameterValue("fm_aout")->load();
    float fm_bout = editor.audioProcessor.params.getRawParameterValue("fm_bout")->load();
    float fm_cout = editor.audioProcessor.params.getRawParameterValue("fm_cout")->load();
    float fm_dout = editor.audioProcessor.params.getRawParameterValue("fm_dout")->load();

    if (fm_aout > 0.f)
    {
        g.setColour(COLOR_A());
        auto bb = Rectangle<float>(r, 2.f).withX(gapX + 0 * (r + gapX)).withY(bounds.getHeight() - 4.f);
        g.fillRect(bb);
    }

    if (fm_bout > 0.f)
    {
        g.setColour(COLOR_B());
        auto bb = Rectangle<float>(r, 2.f).withX(gapX + 1 * (r + gapX)).withY(bounds.getHeight() - 4.f);
        g.fillRect(bb);
    }

    if (fm_cout > 0.f)
    {
        g.setColour(COLOR_C());
        auto bb = Rectangle<float>(r, 2.f).withX(gapX + 2 * (r + gapX)).withY(bounds.getHeight() - 4.f);
        g.fillRect(bb);
    }

    if (fm_dout > 0.f)
    {
        g.setColour(COLOR_D());
        auto bb = Rectangle<float>(r, 2.f).withX(gapX + 3 * (r + gapX)).withY(bounds.getHeight() - 4.f);
        g.fillRect(bb);
    }

}

