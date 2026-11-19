#pragma once
#include <M5Cardputer.h>
#include "visual.hpp"

class VoiceScreen {
public:
    void begin() {
        auto& lcd = M5Cardputer.Display;
        lcd.setRotation(1);
        lcd.fillScreen(bg);
        lcd.setFont(&fonts::lgfxJapanGothic_16);
        lcd.setTextColor(bone, bg);
        lcd.drawString("声の鍵盤", 5, 1);
        lcd.setFont(&fonts::Font0);
        lcd.setTextColor(grid, bg);
        lcd.drawFastHLine(4, 19, 232, grid);
        lcd.setTextColor(bone, bg);
        lcd.drawString("Z X C V B N M ,", 7, 119);
        lcd.setTextColor(tongue, bg);
        lcd.drawString("1:A 2:I 3:U 4:E 5:O", 119, 119);
        frame.setColorDepth(8);
        available = frame.createSprite(240, 92) != nullptr;
    }

    void select(const Keys& keys) {
        static const char* vowels[] = {"あ","い","う","え","お"};
        static const char* notes[] = {"ド","レ","ミ","ファ","ソ","ラ","シ","高ド"};
        auto& lcd = M5Cardputer.Display;
        lcd.fillRect(126, 0, 114, 18, bg);
        lcd.setTextColor(signal, bg);
        lcd.setFont(&fonts::lgfxJapanGothic_16);
        lcd.setCursor(133, 1);
        if (keys.midiPitch>=0) {
            static const char* const onset[]={"","k","s","t","n","h","m","y","w","r","g","z","d","b","p","sh","ch","ts","j","f","N","Q"};
            static const char* const pitchNames[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
            int n=keys.midiPitch;
            char vowel="aiueo"[keys.vowel];
            lcd.printf("%s%s",onset[keys.consonant],keys.palatal?"y":"");
            if (keys.consonant<Nasal) lcd.print(vowel);
            lcd.printf(" %s%d",pitchNames[n%12],n/12-1);
        } else {
            lcd.printf("%s   %s", vowels[keys.vowel], keys.note < 0 ? "休符" : notes[keys.note]);
        }
    }

    void update(const Keys& keys, const int16_t* pcm, size_t count, uint32_t now) {
        // Start the slowed cycle with the onset of each note.
        if (keys.soundingNote()!=lastNote || keys.attack!=lastAttack) {
            noteStarted=now; lastNote=keys.soundingNote(); lastAttack=keys.attack;
        }
        if (!available || now-lastDraw<50) return;
        lastDraw = now;
        frame.fillSprite(bg);
        drawHead(keys.vowel);
        drawFolds(foldGap(keys.soundingNote(), (now - noteStarted) * .001f));
        drawScope(pcm, count);
        frame.pushSprite(&M5Cardputer.Display, 0, 21);
    }
private:
    M5Canvas frame;
    bool available = false;
    uint32_t lastDraw = 0, noteStarted = 0;
    int lastNote = -1;
    unsigned lastAttack=0;
    static constexpr uint16_t bg = 0x0863, bone = 0xe73a, grid = 0x32aa,
        signal = 0x5fd8, tongue = 0xfaee, tissue = 0xa34d;

    void drawHead(int vowel) {
        // Jaw, lips and tongue positions are scaled from the simulator's VPOSE.
        struct Pose { int jaw, lip, tipX, tipY, bodyX, bodyY; };
        static constexpr Pose poses[] = {{9,1,87,43,65,45},{1,0,91,34,73,30},
            {2,1,79,38,57,32},{4,1,89,38,70,36},{5,5,80,40,59,38}};
        const auto& p = poses[vowel];
        frame.drawBezier(42,53,11,26,26,3,55,3,bone);    // occiput and crown
        frame.drawBezier(55,3,84,0,84,14,86,18,bone);   // forehead
        frame.drawLine(86,18,96,28,bone);                 // nose
        frame.drawBezier(96,28,94,32,87,30,bone);
        frame.drawBezier(87,30,88,34,95+p.lip,34,tongue); // upper lip
        frame.drawBezier(94+p.lip,36+p.jaw,84,37+p.jaw,87,40+p.jaw,tongue);
        frame.drawBezier(87,40+p.jaw,87,52+p.jaw,74,50+p.jaw,50,42,bone);
        frame.drawLine(42,49,39,58,bone);
        frame.drawLine(58,51,57,61,bone);
        frame.drawBezier(48,54,46,29,62,23,86,32,grid);  // pharynx and palate
        frame.drawBezier(54,55,54,42,p.bodyX,p.bodyY,p.tipX,p.tipY,tongue);
        frame.drawBezier(54,55,65,49+p.jaw,80,44+p.jaw,p.tipX,p.tipY,tongue);
        frame.drawFastHLine(47,54,8,signal);               // larynx
        frame.drawLine(55,54,112,54,grid);
        frame.drawLine(112,54,131,33,grid);
    }

    void drawFolds(float aperture) {
        // Schematic view from above. The paired pale edges enclose the glottis.
        constexpr int cx = 169, cy = 31;
        frame.drawCircle(cx,cy,28,grid);
        frame.fillEllipse(cx,cy,20,24,tissue);
        frame.fillEllipse(cx,cy,14,19,tongue);
        int gap = lroundf(aperture);
        frame.fillEllipse(cx,cy+1,gap,16,bg);
        frame.drawBezier(cx,cy-16,cx-gap*2,cy,cx,cy+17,bone);
        frame.drawBezier(cx,cy-16,cx+gap*2,cy,cx,cy+17,bone);
        frame.setFont(&fonts::Font0);
        frame.setTextColor(signal);
        frame.drawString("1/160",204,35);
        frame.setFont(&fonts::lgfxJapanGothic_16);
        frame.drawString("声帯",202,15);
    }

    void drawScope(const int16_t* pcm, size_t count) {
        constexpr int y = 77, left = 6, width = 228;
        for (int x = left; x <= left+width; x += 19)
            frame.drawFastVLine(x, y-10, 21, grid);
        frame.drawFastHLine(left, y, width, grid);
        // Trigger at a rising zero crossing when possible, using the actual PCM block.
        const size_t samples = count*3/4;
        size_t start = 0;
        for (size_t i=1; i <= count-samples; ++i) {
            if (pcm[i-1] <= 0 && pcm[i] > 0) { start=i; break; }
        }
        int prevY = y + waveOffset(pcm[start]);
        for (size_t i=1; i<samples; ++i) {
            int nextY = y+waveOffset(pcm[start+i]);
            frame.drawLine(left+(i-1)*width/(samples-1),prevY,
                           left+i*width/(samples-1),nextY,signal);
            prevY = nextY;
        }
    }
};
