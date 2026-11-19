#include <M5Cardputer.h>
#include "screen.hpp"
#include "midi_usb.hpp"

namespace {
Voice voice;
Keys keys, localKeys;
MidiInput midi;
bool usbStarted=false, wasReady=false;
VoiceScreen screen;
constexpr int frames = 128;
int16_t buffers[3][frames];
unsigned nextBuffer = 0;
bool speakerReady = false;
}

void setup() {
    auto cfg = M5.config();
    cfg.internal_mic = false;
    M5Cardputer.begin(cfg,true);
    screen.begin();
    screen.select(keys);

    usbStarted=startMidiUsb();
    if (!usbStarted) M5Cardputer.Display.drawString("USB MIDI error", 78, 1);
    voice.begin();
    auto& speaker = M5Cardputer.Speaker;
    auto audio = speaker.config();
    audio.sample_rate = Voice::rate;
    audio.dma_buf_len = 128;
    audio.dma_buf_count = 4;
    speaker.config(audio);
    speaker.setVolume(100);
    speakerReady = speaker.begin();
    if (!speakerReady) M5Cardputer.Display.drawString("Speaker error", 10, 32);
}

void loop() {
    if (!speakerReady) { delay(100); return; }
    M5Cardputer.update();
    auto& keyboard = M5Cardputer.Keyboard;
    char held[56];
    size_t count = 0;
    for (const auto& point : keyboard.keyList()) {
        if (count < sizeof(held) && point.x < 14 && point.y < 4 &&
            point.x >= 0 && point.y >= 0)
            held[count++] = keyboard.getKeyValue(point).value_first;
    }
    const int lastNote=keys.soundingNote(), lastVowel=keys.vowel;
    const int lastConsonant=keys.consonant;
    bool lastGlide=keys.palatal;
    localKeys.update(held,count);
    if (localKeys.localVowelPressed) midi.selectVowel(localKeys.vowel);
    const bool ready=usbStarted && tud_ready();
    if (!ready && wasReady) midi.disconnect();
    wasReady=ready;
    if (ready) {
        uint8_t packet[4];
        for(int i=0; i<16 && tud_midi_packet_read(packet); ++i) midi.packet(packet);
    }
    keys=localKeys;
    midi.apply(keys);
    auto& speaker = M5Cardputer.Speaker;
    for (int i=0; i<2 && voice.needsAudio(keys) && speaker.isPlaying(0)<2; ++i) {
        auto* pcm = buffers[nextBuffer];
        voice.render(keys, pcm, frames);
        if (!speaker.playRaw(pcm, frames, Voice::rate, false, 1, 0)) break;
        nextBuffer = (nextBuffer + 1) % 3;
    }
    if (keys.soundingNote()!=lastNote || keys.vowel!=lastVowel || keys.consonant!=lastConsonant || keys.palatal!=lastGlide) screen.select(keys);
    screen.update(keys, buffers[(nextBuffer + 2) % 3], frames, millis());
    delay(1);
}
