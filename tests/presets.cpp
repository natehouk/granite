// Play a loud, dynamic test mix through every factory preset; report loudness, peaks, gain reduction.
#include "../plugins/Granite/ParamDefs.hpp"
#include "../plugins/Granite/Presets.hpp"
#include <cstdio>
#include <vector>
using namespace granite;
int main() {
    const int SR = 48000, N = SR * 4; int bad = 0;
    std::vector<float> l(N), r(N); uint32_t s = 3; float lp = 0;
    for (int i = 0; i < N; ++i) { s = s * 1664525u + 1013904223u; const float w = (int32_t)s * (1.f / 2147483648.f); lp += 0.08f * (w - lp);
        const float t = std::fmod(i / (float)SR, 0.5f), hit = std::exp(-t * 25.f);
        l[i] = 0.5f * hit * w + 0.3f * std::sin(2 * kPi * 55 * i / (float)SR) + 0.5f * lp + 0.4f * hit * std::sin(2 * kPi * 70 * i / (float)SR);
        r[i] = 0.9f * l[i] + 0.1f * w * hit; }
    for (uint32_t p = 0; p < kNumPresets; ++p) {
        Engine e; for (uint32_t k = 0; k < kParamCount; ++k) e.p[k] = presets()[p].v[k]; e.setSampleRate(SR);
        std::vector<float> L(N), R(N); for (int i = 0; i < N; i += 512) e.process(l.data() + i, r.data() + i, L.data() + i, R.data() + i, std::min(512, N - i));
        double ss = 0; float pk = 0; bool nf = false;
        for (int i = SR; i < N; ++i) { ss += L[i] * L[i] + R[i] * R[i]; pk = std::max(pk, std::max(std::fabs(L[i]), std::fabs(R[i]))); nf |= !std::isfinite(L[i]); }
        const float rms = 10 * std::log10(ss / (2.0 * (N - SR)) + 1e-20), pkDb = gainToDb(pk), ceil = presets()[p].v[kCeiling];
        const bool ok = !nf && pkDb <= ceil + 0.01f; bad += !ok;
        printf("%-20s %-10s rms %6.1f dB  peak %6.2f dB (out %5.1f)  GR %4.1f dB %s\n", presets()[p].name, presets()[p].category, rms, pkDb, ceil, e.meterGr(), ok ? "" : "<-- CHECK");
    }
    printf("%u presets, %d flagged\n", kNumPresets, bad); return bad;
}
