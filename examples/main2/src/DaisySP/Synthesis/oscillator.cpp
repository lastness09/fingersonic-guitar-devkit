#include "../Utility/dsp.h"
#include "oscillator.h"

using namespace daisysp;
static inline float Polyblep(float phase_inc, float t);

namespace { float sineTable[2049]; }
void Oscillator::InitSineTable()
{
    for(int i=0; i<=2048; ++i) sineTable[i] = sinf(i * (TWOPI_F / 2048.f));
}
float Oscillator::Sine(float phase)
{
    // Normalize negative FM phase to [0,1) before indexing the sine table.
    phase -= static_cast<int>(phase);
    if(phase < 0.f) phase += 1.f;
    const float position = phase * 2048.f;
    const int index = static_cast<int>(position);
    return sineTable[index] + (sineTable[index+1]-sineTable[index])*(position-index);
}

float Oscillator::Process()
{
    float out, t;
    switch(waveform_)
    {
        case WAVE_SIN: out = Sine(phase_); break;
        case WAVE_TRI:
            t   = -1.0f + (2.0f * phase_);
            out = 2.0f * (fabsf(t) - 0.5f);
            break;
        case WAVE_SAW: out = -1.0f * (((phase_ * 2.0f)) - 1.0f); break;
        case WAVE_RAMP: out = ((phase_ * 2.0f)) - 1.0f; break;
        case WAVE_SQUARE: out = phase_ < pw_ ? (1.0f) : -1.0f; break;
        case WAVE_POLYBLEP_TRI:
            t   = phase_;
            out = phase_ < 0.5f ? 1.0f : -1.0f;
            out += Polyblep(phase_inc_, t);
            out -= Polyblep(phase_inc_, fastmod1f(t + 0.5f));
            // Leaky Integrator:
            // y[n] = A + x[n] + (1 - A) * y[n-1]
            out       = phase_inc_ * out + (1.0f - phase_inc_) * last_out_;
            last_out_ = out;
            out *= 4.f; // normalize amplitude after leaky integration
            break;
        case WAVE_POLYBLEP_SAW:
            t   = phase_;
            out = (2.0f * t) - 1.0f;
            out -= Polyblep(phase_inc_, t);
            out *= -1.0f;
            break;
        case WAVE_POLYBLEP_SQUARE:
            t   = phase_;
            out = phase_ < pw_ ? 1.0f : -1.0f;
            out += Polyblep(phase_inc_, t);
            out -= Polyblep(phase_inc_, fastmod1f(t + (1.0f - pw_)));
            out *= 0.707f; // ?
            break;
        default: out = 0.0f; break;
    }
    phase_ += phase_inc_;
    if(phase_ > 1.0f)
    {
        phase_ -= 1.0f;
        eoc_ = true;
    }
    else
    {
        eoc_ = false;
    }
    eor_ = (phase_ - phase_inc_ < 0.5f && phase_ >= 0.5f);

    return out * amp_;
}

float Oscillator::CalcPhaseInc(float f)
{
    return f * sr_recip_;
}

static float Polyblep(float phase_inc, float t)
{
    float dt = phase_inc;
    if(t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    else if(t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    else
    {
        return 0.0f;
    }
}
