#include "../src/midi.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
double level(const std::vector<int16_t>& x){
    double e=0; for(auto s:x)e+=double(s)*s;
    return sqrt(e/x.size());
}
double harmonic(const std::vector<int16_t>& x,double hz){
    double a=0,b=0; for(size_t i=0;i<x.size();++i){
        double phase=i*6.283185307*hz/Voice::rate;
        a+=x[i]*cos(phase);b+=x[i]*sin(phase);
    } return hypot(a,b)/x.size();
}
int main(){
    Voice v; v.begin(); Keys k; k.midiPitch=63; k.attack=1;
    std::vector<int16_t> x(4096);
    assert(v.needsAudio(k));
    v.render(k,x.data(),x.size()); v.render(k,x.data(),x.size());
    assert(harmonic(x,311.127)>3*harmonic(x,329.63));
    k.bend=2; v.render(k,x.data(),x.size()); v.render(k,x.data(),x.size());
    assert(harmonic(x,349.228)>3*harmonic(x,311.127));
    double loud=level(x); k.expression=.25f; v.render(k,x.data(),x.size());
    assert(level(x)<loud*.40);
    k.expression=1; k.consonant=Closure; ++k.attack; v.render(k,x.data(),x.size());
    assert(level(x)<loud*.15);
    k.consonant=K; ++k.attack; v.render(k,x.data(),x.size());
    assert(level(x)>loud*.4);
    // Legato return from a held sokuon must restore the older singing note.
    k.midiPitch=62; k.consonant=Closure; ++k.attack; v.render(k,x.data(),x.size());
    k.midiPitch=63; k.consonant=Open; v.render(k,x.data(),x.size());
    assert(level(x)>loud*.4);
    puts("PASS: chromatic audio, bend, expression, consonant/rest and sustained vowel");
}
