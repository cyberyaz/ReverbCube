#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// RoomCube
// Draws a draggable isometric box. All pixel extents are computed as a
// fraction of the component size so the cube scales with window resizes.
//
//  ex  (half-width)  → roomSize
//  ey  (half-height) → damping (inverted: taller = less damping)
//  ez  (half-depth)  → wet mix
//==============================================================================
class RoomCube : public juce::Component
{
public:
    RoomCube (juce::AudioProcessorValueTreeState& apvts);
    ~RoomCube() override = default;

    void paint   (juce::Graphics& g) override;
    void resized () override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;

private:
    // Iso projection: half-extent e fans out to screen-width ~= 2 * e * 2 * cos(30).
    // maxE is derived so the fully-grown cube never exceeds component bounds.
    //   horizontal limit: (W/2 * 0.80) / (2 * cos30)
    //   vertical limit:   H * 0.52 / 2   (origin sits at 58% down, cube grows upward)
    // We take the min so neither axis clips.
    float maxE() const noexcept
    {
        float fromW = ((float)getWidth()  * 0.40f) / (2.0f * std::cos (juce::degreesToRadians (30.0f)));
        float fromH =  (float)getHeight() * 0.26f;
        return juce::jmin (fromW, fromH);
    }
    float minE()      const noexcept { return maxE() * 0.15f; }
    float getMinDim() const noexcept { return (float)juce::jmin (getWidth(), getHeight()); }

    float paramToExtent (float v) const noexcept;
    float extentToParam (float px) const noexcept;

    juce::Point<float> iso (float x3, float y3, float z3) const noexcept;
    void updateVertices();
    int  hitTestCorner (juce::Point<float> pos) const;
    void pushParams();

    juce::AudioProcessorValueTreeState& apvts;

    // normalised half-extents [0,1] stored so they survive window resize
    float nex = 0.35f;
    float ney = 0.25f;
    float nez = 0.35f;

    juce::Point<float> origin;
    std::array<juce::Point<float>, 8> verts;

    float handleRadius() const noexcept { return getMinDim() * 0.022f; }

    int activeHandle  = -1;
    int hoveredHandle = -1;

    juce::Point<float> dragStart;
    float nexAtDrag = 0.0f, neyAtDrag = 0.0f, nezAtDrag = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoomCube)
};

//==============================================================================
class RoomReverbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit RoomReverbAudioProcessorEditor (RoomReverbAudioProcessor&);
    ~RoomReverbAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    void timerCallback() override;

    RoomReverbAudioProcessor& audioProcessor;
    RoomCube cube;

    juce::Label roomSizeLabel, dampingLabel, wetLabel, widthLabel;
    juce::Label roomSizeVal,   dampingVal,   wetVal,   widthVal;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoomReverbAudioProcessorEditor)
};
