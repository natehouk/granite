// 1.x compatibility: random 1.x settings (2.0 parameters at their defaults) through the frozen 1.0.1 engine and
// the 2.0 engine at 44.1-96 kHz, with mid-stream changes: the outputs must match.
#include "../plugins/Granite/ParamDefs.hpp"
#include "legacy/GraniteEngine1.hpp"
#include <cstdio>
#include <random>
#include <vector>
using namespace granite;
int main() {
    std::mt19937 g(7); std::uniform_real_distribution<float> u(0, 1);
    int bad = 0; float worstAll = -1000;
    for (int sr : { 44100, 48000, 96000 }) for (int trial = 0; trial < 30; ++trial) {
        Engine e; granite1::Engine o;
        for (uint32_t i = 0; i <= kMidiOn; ++i) {
            float lo = 0, hi = 1; bool integer = false, logs = false;
            switch (i) {
            case kGain: lo = 0; hi = 30; break;
            case kCeiling: lo = -30; hi = 0; break;
            case kStyle: hi = 3; integer = true; break;
            case kLookahead: hi = 10; break;
            case kAttack: lo = 0.1f; hi = 10; logs = true; break;
            case kRelease: lo = 1; hi = 2000; logs = true; break;
            case kOversampling: hi = 2; integer = true; break;
            case kDither: case kShaping: case kMeterScale: hi = 3; integer = true; break;
            case kDisplayRange: hi = 2; integer = true; break;
            case kTruePeak: case kMidiOn: integer = true; break;
            default: break;
            }
            float v = logs ? lo * std::pow(hi / lo, u(g)) : lo + (hi - lo) * u(g);
            if (integer) v = std::round(v);
            e.p[i] = v; o.p[i] = v;
        }
        e.setSampleRate((float)sr); o.setSampleRate((float)sr);
        const int N = sr; std::vector<float> l(N), r(N), L1(N), R1(N), L2(N), R2(N);
        for (int i = 0; i < N; ++i) { const float env = (i / (sr / 4)) % 2 ? 1.2f : 0.1f; l[i] = env * (2 * u(g) - 1); r[i] = 0.7f * l[i] + 0.2f * (2 * u(g) - 1); }
        for (int i = 0; i < N; i += 333) {
            const int n = std::min(333, N - i);
            if (i >= N / 2 && i < N / 2 + 333) { e.p[kGain] += 3; o.p[granite1::kGain] += 3; e.p[kOversampling] = o.p[granite1::kOversampling] = (float)(((int)e.p[kOversampling] + 1) % 3); }
            o.process(l.data() + i, r.data() + i, L1.data() + i, R1.data() + i, n);
            e.process(l.data() + i, r.data() + i, L2.data() + i, R2.data() + i, n);
        }
        float worst = 0; for (int i = 0; i < N; ++i) worst = std::max(worst, std::max(std::fabs(L1[i] - L2[i]), std::fabs(R1[i] - R2[i])));
        const float rel = 20 * std::log10(worst / 1.2f + 1e-12f);
        worstAll = std::max(worstAll, rel);
        if (rel > -120 || e.latency() != o.latency()) { ++bad; printf("FAIL sr %d trial %d style %.0f os %.0f: difference %.1f dB below the input, latency %u vs %u\n", sr, trial, e.p[kStyle], e.p[kOversampling], -rel, e.latency(), o.latency()); }
    }
    printf("1.x compatibility: largest difference %.1f dB below the input level, %d failures\n", -worstAll, bad);
    return bad;
}
