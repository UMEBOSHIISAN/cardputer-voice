#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

constexpr float pitchHz[] = {261.6256f,293.6648f,329.6276f,349.2282f,
    391.9954f,440.f,493.8833f,523.2511f};

enum Articulation { Open, K, S, T, N, H, M, Y, W, R, G, Z, D, B, P, Sh, Ch, Ts, J, F, Nasal, Closure };

inline float midiHz(int note) { return 440.f * exp2f((note-69)/12.f); }

struct Keys {
    int note = -1;
    int vowel = 0;

    int midiPitch = -1, consonant = 0;
    unsigned attack = 0;
    bool palatal = false, localVowelPressed = false;
    float bend = 0, modulation = 0, expression = 1, velocity = 1;

    int soundingNote() const {
        static constexpr int scale[] = {60,62,64,65,67,69,71,72};
        return midiPitch >= 0 ? midiPitch : (note < 0 ? -1 : scale[note]);
    }

    void update(const char* held, size_t count) {
        constexpr char notes[] = "zxcvbnm,";
        unsigned now = 0;
        for (size_t i = 0; i < count; ++i) {
            const char key = held[i];
            const char* found = key ? std::strchr(notes, key) : nullptr;
            if (found) now |= 1u << (found - notes);
            if (key >= '1' && key <= '5') now |= 1u << (8 + key - '1');
        }
        const unsigned pressed = now & ~previous;
        localVowelPressed = (pressed >> 8) != 0;
        int retained = 0;
        for (int i = 0; i < depth; ++i) {
            if (now & (1u << order[i])) order[retained++] = order[i];
        }
        depth = retained;
        for (int n = 0; n < 8; ++n) {
            if (pressed & (1u << n)) order[depth++] = n;
        }
        note = depth ? order[depth - 1] : -1;
        for (int v = 0; v < 5; ++v) {
            if (pressed & (1u << (8 + v))) vowel = v;
        }
        previous = now;
    }
private:
    unsigned previous = 0;
    int order[8] = {}, depth = 0;
};

class Voice {
public:
    static constexpr int rate = 22050;
    void begin() {
        // ANAT.vowelRef, from the Japanese Vocal Tract Simulator.
        constexpr float formants[][3] = {{755,1175,2860},{315,2240,3040},
            {375,1360,2280},{500,1770,2620},{500,860,2740}};
        constexpr float bandwidth[] = {130,175,230}, level[] = {1.f,.70f,.32f};

        for (int i = 0; i < period; ++i) sine[i] = sinf(6.2831853f*i/period);
        for (int n = 0; n < 61; ++n) {
            const float frequency = midiHz(36+n);
            increment[n] = uint32_t(double(frequency) * 4294967296.0 / rate);
            for (int v = 0; v < 5; ++v) {
                float wave[period] = {};
                for (int h = 1; h < period/2 && h*frequency < 7000; ++h) {
                    float gain = h == 1 ? .45f : .055f/h;
                    for (int f = 0; f < 3; ++f) {
                        float distance = (h*frequency-formants[v][f])/bandwidth[f];
                        gain += level[f]/(1.f + distance*distance);
                    }
                    for (int i = 0; i < period; ++i)
                        wave[i] += gain*sine[(i*h) % period];
                }
                float peak = 0;
                for (float sample : wave) peak = std::max(peak, fabsf(sample));
                for (int i = 0; i < period; ++i)
                    table[n][v][i] = int16_t(wave[i]*21000.f/peak);
            }
        }
    }

    bool needsAudio(const Keys& keys) const {
        return keys.soundingNote() >= 0 || amplitude > 0.f || rampLeft > 0;
    }

    void render(const Keys& keys, int16_t* out, size_t count) {
        const int note = keys.soundingNote();
        const bool held = note >= 0;
        if (held && amplitude == 0)
            for (int v=0; v<5; ++v) weights[v] = v==keys.vowel;
        // Releasing an overlapping note restores the older note without singing its onset twice.
        if (keys.midiPitch>=0 && pitch!=note-36 && keys.attack==lastAttack) {
            consonant=(keys.consonant==Nasal || keys.consonant==Closure) ? keys.consonant : Open;
            onset=4096; palatal=false;
        }
        if (held && (pitch != note-36 || keys.attack != lastAttack)) {
            pitch = note-36;
            rampStart = lastSample;
            rampLeft = 64;
        }
        if (held && (keys.attack!=lastAttack || keys.midiPitch<0)) {
            if (keys.midiPitch>=0) {
                onset=0;
                consonant=keys.consonant;
                palatal=keys.palatal;
            } else { consonant=Open; palatal=false; }
        }
        lastAttack=keys.attack;
        const float targetLevel = held ? keys.expression*sqrtf(keys.velocity) : 0;
        const double baseStep = increment[pitch]*exp2f(keys.bend/12.f);
        for (size_t i = 0; i < count; ++i) {
            amplitude += (targetLevel - amplitude) * (held ? .009f : .005f);
            if (!held && amplitude < .00002f) amplitude = 0;
            int index = phase >> 25;
            float frac = ((phase >> 9) & 0xffff) / 65536.f;
            float mixed = 0;
            const int vowel = palatal && onset<1000 ? 1 : keys.vowel;
            for (int v = 0; v < 5; ++v) {
                const float target = v == vowel ? 1.f : 0.f;
                weights[v] += (target - weights[v])*.0025f;
                const auto* wave = table[pitch][v];
                mixed += weights[v]*(wave[index] + frac*(wave[(index+1)%period]-wave[index]));
            }
            float value = articulated(mixed,index) * amplitude;
            if (onset<4096) ++onset;
            if (rampLeft) {
                value += (rampStart - value) * (rampLeft / 64.f);
                --rampLeft;
            }
            out[i] = lastSample = int16_t(std::max(-31000.f,std::min(31000.f,value)));
            phase += uint32_t(baseStep*(1.f+.020f*keys.modulation*sine[lfo>>25]));
            lfo += 1071307u; // 5.5 Hz at 22.05 kHz
        }
    }
private:
    float articulated(float vowel, int index) {
        if (consonant==Open) return vowel;
        if (consonant==Closure) return 0;
        const float hum = 14000*sine[index]+2600*sine[(index*2)%period];
        if (consonant==Nasal) return hum;
        if (onset>=2048) return vowel;
        float initial=0;
        int duration=700;
        switch(consonant) {
            case N: case M: initial=hum*(consonant==M?.75f:.9f); duration=1100; break;
            case R: initial=onset<140 ? .14f*vowel : .6f*hum; duration=350; break;
            case Y: case W: initial=table[pitch][consonant==Y?1:2][index]; duration=1050; break;
            default: {
                noise=noise*1664525u+1013904223u;
                float white=(int(noise>>16)-32768)*.45f;
                float hiss=white-previousNoise; previousNoise=white;
                lowNoise+=.18f*(white-lowNoise);
                bool voiced=consonant==G||consonant==D||consonant==B||consonant==Z||consonant==J;
                bool burst=consonant==K||consonant==G||consonant==T||consonant==D||consonant==P||consonant==B;
                duration=burst?600:(consonant==H||consonant==F?1120:1650);
                float excitation=hiss*.6f;
                if (consonant==K||consonant==G||consonant==Sh||consonant==Ch||consonant==J)
                    excitation=.57f*white+.2f*lowNoise;
                if (consonant==P||consonant==B||consonant==F||consonant==H)
                    excitation=consonant==H?.30f*white:1.4f*lowNoise;
                float gain=burst?(onset<140?0.f:std::max(0.f,1.f-(onset-140)/410.f)):1.f;
                if ((consonant==Ch||consonant==Ts||consonant==J)&&onset<180) gain=0;
                initial=excitation*gain+(voiced?.32f*hum:0.f);
            }
        }
        float mix=std::max(0.f,std::min(1.f,(onset-duration*.52f)/(duration*.48f)));
        return initial+(vowel-initial)*mix;
    }
    static constexpr int period=128;
    int16_t table[61][5][period] = {};
    float sine[period] = {};
    uint32_t increment[61] = {}, phase = 0, lfo = 0, noise=1;
    float weights[5] = {1,0,0,0,0}, amplitude = 0, previousNoise=0, lowNoise=0;
    int pitch = 24, rampLeft = 0, consonant=Open, onset=4096;
    unsigned lastAttack=0;
    bool palatal=false;
    int16_t lastSample = 0, rampStart = 0;
};
