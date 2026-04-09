#include "PluginEditor.h"

//==============================================================================
namespace Palette
{
    const juce::Colour bg          { 0xff0d0d14 };
    const juce::Colour floorGrid   { 0xff1a1a2e };
    const juce::Colour faceTop     { 0xff2a2d4a };
    const juce::Colour faceLeft    { 0xff1e2038 };
    const juce::Colour faceRight   { 0xff161828 };
    const juce::Colour edge        { 0xff5b5fff };
    const juce::Colour edgeGlow    { 0xff8b8fff };
    const juce::Colour handleIdle  { 0xff5b5fff };
    const juce::Colour handleHover { 0xffffffff };
    const juce::Colour handleDrag  { 0xffffcc44 };
    const juce::Colour labelText   { 0xff9090c0 };
    const juce::Colour valueText   { 0xffddddff };
    const juce::Colour accent      { 0xff5b5fff };
}

//==============================================================================
// RoomCube
//==============================================================================
RoomCube::RoomCube (juce::AudioProcessorValueTreeState& a) : apvts (a)
{
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
}

//--------------------------------------------------------------
float RoomCube::paramToExtent (float v) const noexcept
{
    return minE() + v * (maxE() - minE());
}

float RoomCube::extentToParam (float px) const noexcept
{
    return juce::jlimit (0.0f, 1.0f, (px - minE()) / (maxE() - minE()));
}

juce::Point<float> RoomCube::iso (float x3, float y3, float z3) const noexcept
{
    float px = (x3 - z3) * std::cos (juce::degreesToRadians (30.0f));
    float py = (x3 + z3) * std::sin (juce::degreesToRadians (30.0f)) - y3;
    return origin + juce::Point<float> (px, py);
}

void RoomCube::updateVertices()
{
    float ex = paramToExtent (nex);
    float ey = paramToExtent (ney);
    float ez = paramToExtent (nez);

    verts[0] = iso (-ex,    0,  ez);
    verts[1] = iso ( ex,    0,  ez);
    verts[2] = iso ( ex, 2*ey,  ez);
    verts[3] = iso (-ex, 2*ey,  ez);
    verts[4] = iso (-ex,    0, -ez);
    verts[5] = iso ( ex,    0, -ez);
    verts[6] = iso ( ex, 2*ey, -ez);
    verts[7] = iso (-ex, 2*ey, -ez);
}

void RoomCube::resized()
{
    // Anchor the origin at 58% down the component — cube grows upward from there
    origin = { getWidth() * 0.5f, getHeight() * 0.58f };

    // Sync normalised extents from APVTS so the cube shape is correct after resize
    nex = *apvts.getRawParameterValue ("roomSize");
    ney = 1.0f - *apvts.getRawParameterValue ("damping");
    nez = *apvts.getRawParameterValue ("wet");

    updateVertices();
}

int RoomCube::hitTestCorner (juce::Point<float> pos) const
{
    float r = handleRadius() * 1.6f;
    for (int i = 0; i < 8; ++i)
        if (verts[i].getDistanceFrom (pos) < r)
            return i;
    return -1;
}

void RoomCube::pushParams()
{
    apvts.getParameter ("roomSize")->setValueNotifyingHost (nex);
    apvts.getParameter ("damping") ->setValueNotifyingHost (1.0f - ney);
    apvts.getParameter ("wet")     ->setValueNotifyingHost (nez);
}

//--------------------------------------------------------------
void RoomCube::mouseDown (const juce::MouseEvent& e)
{
    activeHandle = hitTestCorner (e.position);
    if (activeHandle >= 0)
    {
        dragStart  = e.position;
        nexAtDrag  = nex;
        neyAtDrag  = ney;
        nezAtDrag  = nez;
    }
}

void RoomCube::mouseDrag (const juce::MouseEvent& e)
{
    if (activeHandle < 0) return;

    juce::Point<float> delta = e.position - dragStart;
    float dim = getMinDim();

    // Sensitivity: fraction of component size per pixel dragged
    float sens = 1.0f / (dim * 0.5f);

    float dH =  delta.x * sens;
    float dV = -delta.y * sens;   // screen y inverted

    // Right-side corners mirror horizontal direction
    float hSign = (activeHandle == 1 || activeHandle == 5) ? -1.0f : 1.0f;

    nex = juce::jlimit (0.0f, 1.0f, nexAtDrag + dH * hSign);
    ney = juce::jlimit (0.0f, 1.0f, neyAtDrag + dV);
    nez = juce::jlimit (0.0f, 1.0f, nezAtDrag + dH * hSign);

    updateVertices();
    pushParams();
    repaint();
}

void RoomCube::mouseUp (const juce::MouseEvent&)
{
    activeHandle = -1;
    repaint();
}

void RoomCube::mouseMove (const juce::MouseEvent& e)
{
    int prev = hoveredHandle;
    hoveredHandle = hitTestCorner (e.position);
    if (hoveredHandle != prev) repaint();
}

//--------------------------------------------------------------
void RoomCube::paint (juce::Graphics& g)
{
    float w = (float) getWidth();
    float h = (float) getHeight();

    // Background grid
    g.setColour (Palette::floorGrid);
    float gs = getMinDim() * 0.055f;
    for (float x = 0; x < w; x += gs)  g.drawVerticalLine   ((int)x, 0, h);
    for (float y = 0; y < h; y += gs)  g.drawHorizontalLine ((int)y, 0, w);

    // Sync extents from params each paint (handles host automation)
    nex = *apvts.getRawParameterValue ("roomSize");
    ney = 1.0f - *apvts.getRawParameterValue ("damping");
    nez = *apvts.getRawParameterValue ("wet");
    updateVertices();

    auto& v = verts;

    auto makeFace = [&](int a, int b, int c, int d)
    {
        juce::Path p;
        p.startNewSubPath (v[a]);
        p.lineTo (v[b]);
        p.lineTo (v[c]);
        p.lineTo (v[d]);
        p.closeSubPath();
        return p;
    };

    // Filled faces
    g.setColour (Palette::faceTop);   g.fillPath (makeFace (7, 6, 2, 3));
    g.setColour (Palette::faceLeft);  g.fillPath (makeFace (4, 0, 3, 7));
    g.setColour (Palette::faceRight); g.fillPath (makeFace (1, 5, 6, 2));

    // Edges with glow
    float thick = getMinDim() * 0.004f;
    auto drawEdge = [&](int a, int b, juce::Colour col, float t)
    {
        juce::Path ep;
        ep.startNewSubPath (v[a]);
        ep.lineTo (v[b]);
        g.setColour (col.withAlpha (0.35f));
        g.strokePath (ep, juce::PathStrokeType (t + getMinDim() * 0.008f));
        g.setColour (col);
        g.strokePath (ep, juce::PathStrokeType (t));
    };

    drawEdge (0, 1, Palette::edgeGlow, thick * 1.5f);
    drawEdge (1, 2, Palette::edgeGlow, thick * 1.5f);
    drawEdge (2, 3, Palette::edgeGlow, thick * 1.5f);
    drawEdge (3, 0, Palette::edgeGlow, thick * 1.5f);
    drawEdge (3, 7, Palette::edge, thick);
    drawEdge (7, 6, Palette::edge, thick);
    drawEdge (6, 2, Palette::edge, thick);
    drawEdge (5, 6, Palette::edge, thick);
    drawEdge (4, 7, Palette::edge, thick);
    drawEdge (4, 5, Palette::edge, thick);
    drawEdge (1, 5, Palette::edge, thick);
    drawEdge (0, 4, Palette::edge, thick);

    // Floor shadow
    {
        juce::Path shadow;
        shadow.startNewSubPath (v[0]);
        shadow.lineTo (v[1]);
        shadow.lineTo (v[5]);
        shadow.lineTo (v[4]);
        shadow.closeSubPath();
        g.setColour (Palette::accent.withAlpha (0.12f));
        g.fillPath (shadow);
    }

    // Dimension annotations
    float fontSize = getMinDim() * 0.045f;
    g.setFont (juce::Font (fontSize));
    g.setColour (Palette::labelText);

    float rs = *apvts.getRawParameterValue ("roomSize");
    float dm = *apvts.getRawParameterValue ("damping");
    float wt = *apvts.getRawParameterValue ("wet");

    auto midFrontBottom  = (v[0] + v[1]) * 0.5f;
    auto midFrontLeft    = (v[0] + v[3]) * 0.5f;
    auto midRightPillar  = (v[1] + v[5]) * 0.5f;
    int fw = (int)(fontSize * 2.5f);
    int fh = (int)(fontSize * 1.4f);

    g.drawText (juce::String (rs, 2),
                (int)midFrontBottom.x - fw/2, (int)midFrontBottom.y + 3, fw, fh,
                juce::Justification::centred);
    g.drawText (juce::String (1.0f - dm, 2),
                (int)midFrontLeft.x - fw - 2, (int)midFrontLeft.y - fh/2, fw, fh,
                juce::Justification::centredRight);
    g.drawText (juce::String (wt, 2),
                (int)midRightPillar.x + 3, (int)midRightPillar.y - fh/2, fw, fh,
                juce::Justification::centredLeft);

    // Corner handles
    float hr = handleRadius();
    for (int i = 0; i < 8; ++i)
    {
        juce::Colour col = (i == activeHandle)  ? Palette::handleDrag
                         : (i == hoveredHandle) ? Palette::handleHover
                                                : Palette::handleIdle;
        g.setColour (col.withAlpha (0.25f));
        g.fillEllipse (v[i].x - hr * 1.6f, v[i].y - hr * 1.6f, hr * 3.2f, hr * 3.2f);
        g.setColour (col.withAlpha (0.85f));
        g.fillEllipse (v[i].x - hr * 0.6f, v[i].y - hr * 0.6f, hr * 1.2f, hr * 1.2f);
    }
}

//==============================================================================
// Editor
//==============================================================================
RoomReverbAudioProcessorEditor::RoomReverbAudioProcessorEditor (RoomReverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), cube (p.apvts)
{
    setSize (520, 420);
    // Let the host handle resizing — no JUCE resize grip (2nd arg false)
    // which prevents the double-constrainer fight that causes bounciness
    setResizable (true, false);

    addAndMakeVisible (cube);

    auto setup = [&](juce::Label& lbl, const juce::String& text, bool isValue)
    {
        lbl.setText (text, juce::dontSendNotification);
        lbl.setJustificationType (juce::Justification::centred);
        lbl.setColour (juce::Label::textColourId,
                       isValue ? Palette::valueText : Palette::labelText);
        addAndMakeVisible (lbl);
    };

    setup (roomSizeLabel, "ROOM SIZE", false);
    setup (dampingLabel,  "DAMPING",   false);
    setup (wetLabel,      "WET",       false);
    setup (widthLabel,    "WIDTH",     false);
    setup (roomSizeVal,   "0.50", true);
    setup (dampingVal,    "0.50", true);
    setup (wetVal,        "0.33", true);
    setup (widthVal,      "1.00", true);

    startTimerHz (30);
}

RoomReverbAudioProcessorEditor::~RoomReverbAudioProcessorEditor()
{
    stopTimer();
}

void RoomReverbAudioProcessorEditor::timerCallback()
{
    auto fmt = [](float v) { return juce::String (v, 2); };
    roomSizeVal.setText (fmt (*audioProcessor.apvts.getRawParameterValue ("roomSize")), juce::dontSendNotification);
    dampingVal .setText (fmt (*audioProcessor.apvts.getRawParameterValue ("damping")),  juce::dontSendNotification);
    wetVal     .setText (fmt (*audioProcessor.apvts.getRawParameterValue ("wet")),      juce::dontSendNotification);
    widthVal   .setText (fmt (*audioProcessor.apvts.getRawParameterValue ("width")),    juce::dontSendNotification);
    cube.repaint();
}

void RoomReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Palette::bg);

    float titleSize = getHeight() * 0.038f;
    g.setColour (Palette::valueText);
    g.setFont (juce::Font (titleSize, juce::Font::bold));
    g.drawText ("ROOM REVERB", 0, (int)(titleSize * 0.4f), getWidth(), (int)(titleSize * 1.4f),
                juce::Justification::centred);

    int divY = (int)(titleSize * 2.2f);
    g.setColour (Palette::accent.withAlpha (0.4f));
    g.drawHorizontalLine (divY, 20.0f, (float)(getWidth() - 20));
}

void RoomReverbAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Title strip
    int titleH = (int)(getHeight() * 0.09f);
    area.removeFromTop (titleH);

    // Label strip at bottom — scales with height
    int labelH = (int)(getHeight() * 0.11f);
    auto labelStrip = area.removeFromBottom (labelH);

    float labelFontSize = labelH * 0.28f;
    float valueFontSize = labelH * 0.36f;

    int colW = labelStrip.getWidth() / 4;
    auto doCol = [&](juce::Label& name, juce::Label& val)
    {
        name.setFont (juce::Font (labelFontSize));
        val .setFont (juce::Font (valueFontSize));
        auto col = labelStrip.removeFromLeft (colW);
        name.setBounds (col.removeFromTop (labelH / 2));
        val .setBounds (col);
    };
    doCol (roomSizeLabel, roomSizeVal);
    doCol (dampingLabel,  dampingVal);
    doCol (wetLabel,      wetVal);
    doCol (widthLabel,    widthVal);

    cube.setBounds (area.reduced (8));
}
