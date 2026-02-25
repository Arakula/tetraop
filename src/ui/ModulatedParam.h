#pragma once
#include "JuceHeader.h"

class ModulatedParam : public juce::Component
{
public:
    juce::String paramId;
    juce::String modId; // selected modulator applied to this param
    float modValue = 0.0f; // final value of the param with modulation applied
    bool modulated = false; // whether this param is modulated by any source
    bool modBipolar = false;
    bool showDragAndDrop = false;
    bool voiceActive = false; // if there is no voice active, the modvalue should be drawn at zero offset (no change)
    Colour dragAndDropColour{};
    Colour modColor{};

    ModulatedParam(juce::String paramId) : paramId(paramId)
    {
        setName(paramId);
    }

    virtual void setModId(String mid) = 0;
    virtual ~ModulatedParam() = default;
};