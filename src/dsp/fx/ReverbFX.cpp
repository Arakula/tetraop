#include "./Reverb.h"
#include "../../PluginProcessor.h"


ReverbFX::ReverbFX ( RipplerAudioProcessor& p, int _layer )
    : FX ( p, FX::Reverb, _layer )
{
    modeParam        = audioProcessor.params.getRawParameterValue ( prefix + "mode" );

    audioProcessor.params.addParameterListener ( prefix + "mode", this );
    audioProcessor.params.addParameterListener ( prefix + "on", this ););

    rutaVerb = std::make_unique<reFX::RutaVerb> ();
    miniVerb = std::make_unique<MiniVerb>(p, _layer);
}

Reverb::~Reverb ()
{
    audioProcessor.params.removeParameterListener ( prefix + "mode", this );
    audioProcessor.params.removeParameterListener ( prefix + "on", this );
}

void Reverb::parameterChanged ( const juce::String& /*paramId*/, float /*value*/ )
{
    clear ();
}

void Reverb::prepare ( float _srate )
{
    srate = _srate;

    miniVerb->prepare(_srate);
    rutaVerb->setSamplerate ( _srate );
}

void Reverb::processBlock ( float* left, float* right, int nsamps, int /* blockOffset */, bool /* audioRate */)
{
    //auto mode = fast::clamp ( juce::roundToInt ( modeParam->load () ), 0, 5 );
    miniVerb->processBlock(left, right, nsamps, 0, false);
    return;
}

void Reverb::clear ()
{
    miniVerb->clear();
    rutaVerb->kill ();
}
