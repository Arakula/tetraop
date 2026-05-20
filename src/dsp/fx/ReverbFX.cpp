#include "./ReverbFX.h"
#include "../../PluginProcessor.h"


ReverbFX::ReverbFX ( TetraOPAudioProcessor& p)
    : FX ( p, FX::Reverb)
{
    modeParam        = audioProcessor.params.getRawParameterValue ( prefix + "mode" );

    audioProcessor.params.addParameterListener ( prefix + "mode", this );
    audioProcessor.params.addParameterListener ( prefix + "on", this );

    tetraVerb = std::make_unique<TetraVerb>();
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

    tetraVerb->prepare(_srate);
}

void ReverbFX::processBlock ( float* left, float* right, int nsamps, int /* blockOffset */, bool /* audioRate */)
{
    //auto mode = fast::clamp ( juce::roundToInt ( modeParam->load () ), 0, 5 );
    float size = audioProcessor.params.getRawParameterValue("fx_reverb_revsize")->load();
    tetraVerb->setSize(size);

    float decay = audioProcessor.params.getRawParameterValue("fx_reverb_decay")->load();
    tetraVerb->setDecay(decay);

    tetraVerb->processBlock(left, right, nsamps);
}

void ReverbFX::clear ()
{
    //miniVerb->clear();
}
