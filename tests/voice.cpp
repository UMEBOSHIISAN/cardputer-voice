#include "../src/voice.hpp"
#include <string>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

static void press(Keys &keys, const char *held) {
    keys.update(held, std::char_traits<char>::length(held));
}

static double component(const std::vector<int16_t>& pcm, double hz) {
    double x = 0, y = 0;
    for (size_t i = 0; i < pcm.size(); ++i) {
        double phase = 6.28318530718 * hz * i / Voice::rate;
        x += pcm[i] * cos(phase);
        y += pcm[i] * sin(phase);
    }
    return hypot(x, y) / pcm.size();
}

int main() {
    Keys keys;
    press(keys, "z"); assert(keys.note == 0);
    press(keys, "z4"); assert(keys.note == 0 && keys.vowel == 3);
    press(keys, "z"); assert(keys.note == 0 && keys.vowel == 3);
    press(keys, "zx"); assert(keys.note == 1);
    press(keys, "z"); assert(keys.note == 0);
    press(keys, "5"); assert(keys.note == -1 && keys.vowel == 4);
    const char notes[] = "zxcvbnm,";
    const double pitch[] = {261.626,293.665,329.628,349.228,391.995,440,493.883,523.251};
    Voice voice;
    voice.begin();
    assert(!voice.needsAudio(Keys{}));
    std::vector<int16_t> pcm(4096);
    for (int n = 0; n < 8; ++n) {
        keys.update(notes+n, 1);
        assert(keys.note == n);
        for (int v = 0; v < 5; ++v) {
            char chord[] = {notes[n], static_cast<char>('1'+v)};
            keys.update(chord, 2);
            assert(keys.vowel == v && keys.note == n);
            voice.render(keys, pcm.data(), pcm.size());
            voice.render(keys, pcm.data(), pcm.size());
            assert(component(pcm,pitch[n]) > 70);
            assert(component(pcm,pitch[n]) > 3 * component(pcm,pitch[n]*1.06));
            assert(*std::max_element(pcm.begin(),pcm.end()) < 30000);
            // A emphasizes the third harmonic on C4, I the region near 2.2 kHz.
            if (n == 0 && v == 0) assert(component(pcm,785) > 2*component(pcm,2093));
            if (n == 0 && v == 1) assert(component(pcm,2093) > 2*component(pcm,785));
        }
    }
    // Identical held C4 voices must remain phase-aligned at a vowel change.
    Voice reference;
    reference.begin();
    Voice switching;
    switching.begin();
    Keys sustained;
    press(sustained, "z");
    reference.render(sustained,pcm.data(),pcm.size());
    switching.render(sustained,pcm.data(),pcm.size());
    int16_t baseline[128], changed[128];
    reference.render(sustained,baseline,128);
    press(sustained,"z5");
    switching.render(sustained,changed,128);
    assert(std::abs(int(baseline[0])-changed[0]) < 150);
    double transitionEnergy = 0;
    for (auto sample : changed) transitionEnergy += double(sample)*sample;
    assert(std::sqrt(transitionEnergy/128) > 1000);
    // All five vowel shapes must be distinguishable at one fixed pitch.
    double signatures[5][12] = {};
    for (int v=0; v<5; ++v) {
        char held[] = {'z',static_cast<char>('1'+v)};
        sustained.update(held,2);
        switching.render(sustained,pcm.data(),pcm.size());
        switching.render(sustained,pcm.data(),pcm.size());
        double energy=0;
        for (int h=0; h<12; ++h) {
            double a=component(pcm,261.626*(h+1));
            signatures[v][h]=a;
            energy+=a*a;
        }
        for (double &a : signatures[v]) a/=sqrt(energy);
        for (int prior=0; prior<v; ++prior) {
            double distance=0;
            for (int h=0; h<12; ++h) distance+=std::pow(signatures[v][h]-signatures[prior][h],2);
            assert(distance > .045);
        }
    }
    // A new held pitch must not jump directly to another table's full-scale sample.
    press(sustained,"z");
    switching.render(sustained,pcm.data(),pcm.size()-37);
    int before=pcm[pcm.size()-38];
    press(sustained,"z,");
    switching.render(sustained,changed,128);
    assert(std::abs(before-int(changed[0])) <= 200);
    press(keys, "");
    voice.render(keys,pcm.data(),pcm.size());
    assert(!voice.needsAudio(keys));
    assert(std::all_of(pcm.end()-512,pcm.end(),[](int16_t x){return x == 0;}));
    std::puts("PASS: eight pitches, five latched vowels, overlapping keys, PCM spectra and release");
}
