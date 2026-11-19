#pragma once
#include "voice.hpp"

inline int waveOffset(int16_t sample) {
    return std::max(-10, std::min(10, -int(sample) / 2350));
}

// 160x slower than the played note, so the motion is visible at 20 fps.
inline float foldGap(int note, float seconds) {
    if (note < 0) return 6.f;
    return 1.f + 4.f * (1.f - cosf(seconds * midiHz(note) * (6.2831853f / 160.f)));
}
