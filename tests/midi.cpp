#include "../src/midi.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

int main() {
    MidiInput m;
    Keys local;
    auto state = [&] { Keys s=local; m.apply(s); return s; };
    auto msg=[&](int status,int a,int b){
        uint8_t p[]={uint8_t(status>>4),uint8_t(status),uint8_t(a),uint8_t(b)};
        m.packet(p);
    };
    msg(0x91,61,100); assert(state().soundingNote()==-1);
    msg(0x90,61,100); assert(state().soundingNote()==61);
    msg(0xb0,20,110); assert(state().vowel==4);
    msg(0xe0,127,127); assert(state().bend>1.99f);
    msg(0xb0,1,90); assert(state().modulation>0.6f);
    msg(0xb0,11,56); assert(state().expression<.46f);
    msg(0x90,66,80); msg(0x80,66,0); assert(state().soundingNote()==61);
    msg(0x90,61,0); assert(state().soundingNote()==-1);
    assert(state().bend>1.99f && state().modulation>.6f);
    // Local development SysEx, two morae: ka, ru. Install only at EOX.
    auto packet=[&](int cin,int a,int b,int c){
        uint8_t p[]={uint8_t(cin),uint8_t(a),uint8_t(b),uint8_t(c)}; m.packet(p);
    };
    packet(4,0xf0,0x7d,'V'); packet(4,'K',1,2);
    packet(4,1,0,9); packet(6,2,0xf7,0);
    msg(0x90,60,100); assert(state().consonant==1 && state().vowel==0);
    unsigned attack=state().attack;
    msg(0xb0,20,127); assert(state().vowel==4 && state().attack==attack);
    msg(0x90,62,80); assert(state().consonant==9 && state().vowel==2);
    msg(0x80,62,0); assert(state().soundingNote()==60);
    msg(0xb0,123,0); assert(state().soundingNote()==-1);
    packet(4,0xf0,0x7d,'V'); packet(4,'K',1,1); packet(7,127,0,0xf7);
    msg(0x90,64,100); assert(state().consonant==1);
    m.disconnect(); assert(state().soundingNote()==-1);
    local.update("x",1); assert(state().soundingNote()==62);
    puts("PASS: chromatic notes, input ownership, expression, CC, atomic lyric load, lyric stepping and disconnect");
}
