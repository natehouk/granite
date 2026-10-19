// CPU: heavy limiting with true-peak meters and weighted dither, at 1x, 2x and 4x.
#include "../plugins/Granite/ParamDefs.hpp"
#include <chrono>
#include <cstdio>
using namespace granite;
int main() {
    const int SR = 48000;
    static float in[SR], L[SR], R[SR]; uint32_t r = 1; for (float& x : in) { r = r * 1664525u + 1013904223u; x = 0.5f * ((int32_t)r * (1.f / 2147483648.f)); }
    for (int os = 0; os < 3; ++os) {
        Engine e; e.p[kOversampling] = (float)os; e.p[kGain] = 12; e.p[kLookahead] = 5; e.p[kDither] = 1; e.p[kShaping] = 3; e.p[kTruePeak] = 1; e.setSampleRate(SR);
        const int secs = 20; auto t0 = std::chrono::steady_clock::now();
        for (int k = 0; k < secs; ++k) for (int b = 0; b < SR; b += 256) e.process(in + b, in + b, L + b, R + b, 256);
        const double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        printf("%dx: %d s of audio in %.3f s: %.2f%% of one core\n", os == 0 ? 1 : (os == 1 ? 2 : 4), secs, el, 100 * el / secs);
    }
    return 0;
}
