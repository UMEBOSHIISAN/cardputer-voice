#pragma once
#include "voice.hpp"


class MidiInput {
    struct Mora { uint8_t consonant, shape; };
    struct Held { uint8_t pitch, velocity, references; Mora mora; };
    Held held[61] = {};
    uint8_t depth = 0, length = 0, cursor = 0, vowel = 0;
    bool selected = false;
    Mora phrase[96] = {};
    uint8_t wire[5 + 2*96] = {};
    size_t used = 0;
    bool receiving = false;
    unsigned attack = 0;
    float bend = 0, modulation = 0, expression = 1;

    void byte(uint8_t b) {
        if (b == 0xf0) { used = 0; receiving = true; return; }
        if (!receiving) return;
        if (b == 0xf7) {
            receiving = false;
            if (used < 5 || wire[0]!=0x7d || wire[1]!='V' || wire[2]!='K' ||
                wire[3]!=1 || wire[4]>96 || used != size_t(5+2*wire[4])) return;
            for (size_t i=5; i<used; i+=2)
                if (wire[i]>Closure || wire[i+1]>9) return;
            length = wire[4]; cursor = 0;
            for (int i=0;i<length;++i) phrase[i]={wire[5+i*2],wire[6+i*2]};
        } else if (b>=128 || used==sizeof(wire)) receiving=false;
        else wire[used++]=b;
    }

    void note(uint8_t pitch, uint8_t velocity) {
        if (pitch < 36 || pitch > 96) return;
        int pos = 0;
        while (pos < depth && held[pos].pitch != pitch) ++pos;
        if (!velocity && pos == depth) return;
        uint8_t refs = 0;
        if (pos < depth) {
            refs = held[pos].references;
            if (!velocity && refs > 1) { --held[pos].references; return; }
            for (int i=pos+1; i<depth; ++i) held[i-1]=held[i];
            --depth;
        }
        if (velocity) {
            Mora mora = {Open,vowel};
            if (length) { mora = phrase[cursor]; cursor = (cursor+1)%length; }
            held[depth++] = {pitch,velocity,uint8_t(std::min(255,int(refs)+1)),mora};
            vowel=mora.shape%5; selected=true; ++attack;
        } else if (pos == depth && depth) {
            vowel=held[depth-1].mora.shape%5;
        }
    }

public:
    void selectVowel(int v) {
        vowel=v; selected=true;
        if (depth) held[depth-1].mora.shape=v;
    }
    void disconnect() { depth=0; receiving=false; bend=0; modulation=0; expression=1; cursor=0; }

    void packet(const uint8_t* p) {
        if (p[0]>>4) return; // Only the advertised cable.
        const int cin=p[0]&15;
        if (cin>=4 && cin<=7) {
            int size = cin==4 ? 3 : cin-4;
            for (int i=1;i<=size;++i) byte(p[i]);
            return;
        }
        if (cin==15) return; // Interleaved MIDI clock does not end a SysEx.
        receiving=false;
        if ((p[1]&15)!=0 || (p[1]>>4)!=cin || p[2]>=128 || p[3]>=128) return;
        if (cin==8 || cin==9) note(p[2],cin==9 ? p[3] : 0);
        if (cin==14) bend=((p[3]*128+p[2])-8192)*(2.f/8192.f);
        if (cin!=11) return;
        switch(p[2]) {
            case 1: modulation=p[3]/127.f; break;
            case 11: expression=p[3]/127.f; break;
            case 20: selectVowel(p[3]*5/128); break;
            case 21: if(p[3]>=64) cursor=0; break;
            case 120: case 123: depth=0; break;
            case 121: bend=0; modulation=0; expression=1; break;
        }
    }

    void apply(Keys& keys) const {
        if (selected) keys.vowel=vowel;
        if (depth || keys.note<0) {
            keys.bend=bend; keys.modulation=modulation; keys.expression=expression;
        }
        if (!depth) return;
        const auto& active=held[depth-1];
        keys.midiPitch=active.pitch;
        keys.velocity=active.velocity/127.f;
        keys.attack=attack; keys.consonant=active.mora.consonant;
        keys.palatal=active.mora.shape>=5;
    }
};
