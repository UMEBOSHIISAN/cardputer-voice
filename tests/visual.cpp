#include "../src/visual.hpp"
#include <cassert>
#include <cstdio>
int main() {
    assert(waveOffset(0) == 0);
    assert(waveOffset(18000) <= -6 && waveOffset(-18000) >= 6);
    assert(waveOffset(-32768) <= 10 && waveOffset(32767) >= -10);
    assert(foldGap(-1,0) == foldGap(-1,1));
    assert(foldGap(0,0) != foldGap(0,.25f));
    assert(foldGap(0,.25f) != foldGap(7,.25f));
    puts("PASS: signed PCM trace, bounded screen range, slowed pitch-driven vocal folds");
}
