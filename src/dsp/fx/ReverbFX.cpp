#include "./ReverbFX.h"
#include "../../PluginProcessor.h"


ReverbFX::ReverbFX ( TetraOPAudioProcessor& p)
    : FX ( p, FX::Reverb)
{
    modeParam        = audioProcessor.params.getRawParameterValue ( prefix + "mode" );

    audioProcessor.params.addParameterListener ( prefix + "mode", this );
    audioProcessor.params.addParameterListener ( prefix + "on", this );

    miniVerb = std::make_unique<MiniVerb>(p);
}

ReverbFX::~ReverbFX ()
{
    audioProcessor.params.removeParameterListener ( prefix + "mode", this );
    audioProcessor.params.removeParameterListener ( prefix + "on", this );
}

void ReverbFX::parameterChanged ( const juce::String& /*paramId*/, float /*value*/ )
{
    clear ();
}

void ReverbFX::prepare ( float _srate )
{
    srate = _srate;

    miniVerb->prepare(_srate);
}

void ReverbFX::processBlock ( float* left, float* right, int nsamps, int /* blockOffset */, bool /* audioRate */)
{
    //auto mode = fast::clamp ( juce::roundToInt ( modeParam->load () ), 0, 5 );
    miniVerb->processBlock(left, right, nsamps, 0, false);
    return;
}

void ReverbFX::clear ()
{
    miniVerb->clear();
}
