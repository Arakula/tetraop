#include "./ReverbFX.h"
#include "../../PluginProcessor.h"


ReverbFX::ReverbFX ( TetraOPAudioProcessor& p)
    : FX ( p, FX::Reverb)
{
    modeParam        = audioProcessor.params.getRawParameterValue ( prefix + "mode" );

    audioProcessor.params.addParameterListener ( prefix + "mode", this );
    audioProcessor.params.addParameterListener ( prefix + "on", this );
}

ReverbFX::~ReverbFX ()
{
    audioProcessor.params.removeParameterListener ( prefix + "mode", this );
    audioProcessor.params.removeParameterListener ( prefix + "on", this );
}

void ReverbFX::parameterChanged ( const juce::String& /*paramId*/, float /*value*/ )
{
    clear ();
    mverb.reset();
}

void ReverbFX::prepare ( float _srate )
{
    srate = _srate;

    mverb.setSampleRate(srate);
}

void ReverbFX::processBlock(float* left, float* right, int nsamps, int /* blockOffset */, bool /* audioRate */)
{
    float damp = audioProcessor.params.getRawParameterValue("fx_reverb_damp")->load();
    mverb.setParameter(MVerb<float>::DAMPINGFREQ, damp);
    float lowpass = audioProcessor.params.getRawParameterValue("fx_reverb_lowpass")->load();
    mverb.setParameter(MVerb<float>::BANDWIDTHFREQ, lowpass);
    float density = audioProcessor.params.getRawParameterValue("fx_reverb_density")->load();
    mverb.setParameter(MVerb<float>::DENSITY, density);
    float predel = audioProcessor.params.getRawParameterValue("fx_reverb_predel")->load();
    mverb.setParameter(MVerb<float>::PREDELAY, predel);
    float el = audioProcessor.params.getRawParameterValue("fx_reverb_earlylate")->load();
    mverb.setParameter(MVerb<float>::EARLYMIX, el);
    float mix = audioProcessor.params.getRawParameterValue("fx_reverb_mix")->load();
    mverb.setParameter(MVerb<float>::MIX, mix);
    mverb.setParameter(MVerb<float>::GAIN, 1.f);
    float size_ = audioProcessor.params.getRawParameterValue("fx_reverb_revsize")->load();
    if (size != size_)
    {
        size = size_;
        mverb.setParameter(MVerb<float>::SIZE, size);
    }
    float decay = audioProcessor.params.getRawParameterValue("fx_reverb_decay")->load();
    mverb.setParameter(MVerb<float>::DECAY, decay);

    gin::ScratchBuffer outbuf(2, nsamps);

    float* ins[] = { left, right };
    float* outs[] = { outbuf.getWritePointer(0), outbuf.getWritePointer(1) };

    mverb.process(ins, outs, nsamps);

    for (int i = 0; i < nsamps; ++i)
    {
        left[i] = outs[0][i];
        right[i] = outs[1][i];
    }
}

void ReverbFX::clear ()
{
    mverb.reset();  
}
