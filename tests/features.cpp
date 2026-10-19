// Feature tests for Granite: the ceiling holds for every style/oversampling/look-ahead/attack setting
// and sample rate; latency is exact; transparent below the ceiling; gain reduction amount; linking;
// release; true-peak limiting and metering; dither bit depth and noise shaping; click-free switching.
#include "../plugins/Granite/ParamDefs.hpp"
#include "../plugins/Granite/Presets.hpp"
#include <cstdio>
#include <vector>
using namespace granite;

static int fails = 0;
static void check(bool ok, const char* fmt, double a = 0, double b = 0, double c = 0) {
    printf("%s  ", ok ? "PASS" : "FAIL"); printf(fmt, a, b, c); printf("\n"); fails += !ok;
}
static void run(Engine& e, const std::vector<float>& l, const std::vector<float>& r, std::vector<float>& L, std::vector<float>& R, int block = 256) {
    L.resize(l.size()); R.resize(l.size());
    for (size_t i = 0; i < l.size(); i += block) e.process(l.data() + i, r.data() + i, L.data() + i, R.data() + i, (uint32_t)std::min<size_t>(block, l.size() - i));
}
static std::vector<float> music(int n, float fs, uint32_t seed = 3) {     // drums-like bursts + bass + noise
    std::vector<float> v(n); uint32_t r = seed; float lp = 0;
    for (int i = 0; i < n; ++i) { r = r * 1664525u + 1013904223u; const float w = (int32_t)r * (1.f / 2147483648.f); lp += 0.08f * (w - lp);
        const float t = std::fmod(i / fs, 0.5f), hit = std::exp(-t * 30.f);
        v[i] = 0.5f * hit * w + 0.35f * std::sin(2 * kPi * 55 * i / fs) + 0.15f * lp * 4.f + 0.6f * hit * std::sin(2 * kPi * 80 * i / fs * (1 + 2 * hit)); }
    return v;
}
static float peakDb(const std::vector<float>& v, size_t a) { float p = 0; for (size_t i = a; i < v.size(); ++i) p = std::max(p, std::fabs(v[i])); return gainToDb(p); }
static float truePeakDb(const std::vector<float>& v, size_t a) {    // 8x windowed-sinc interpolation, independent of the engine
    float p = 0;
    for (size_t i = a + 32; i + 32 < v.size(); ++i) for (int ph = 0; ph < 8; ++ph) {
        const double t = ph / 8.0; double s = 0;
        for (int k = -31; k <= 32; ++k) { const double x = k - t; const double sinc = std::fabs(x) < 1e-9 ? 1 : std::sin(3.14159265358979 * x) / (3.14159265358979 * x);
            const double w = 0.42 + 0.5 * std::cos(3.14159265358979 * x / 33) + 0.08 * std::cos(2 * 3.14159265358979 * x / 33); s += v[i + k] * sinc * w; }
        p = std::max(p, (float)std::fabs(s)); }
    return gainToDb(p);
}

int main() {
    const float fs = 48000;
    // 1. the ceiling holds (sample peaks) everywhere
    {   float worst = -100; int cases = 0;
        for (float sr : { 44100.f, 48000.f, 96000.f }) {
            const std::vector<float> l = music((int)sr * 2, sr, 3), r = music((int)sr * 2, sr, 9);
            for (int os = 0; os < 3; ++os) for (int st = 0; st < 4; ++st) for (float la : { 0.f, 1.f, 5.f }) for (float at : { 0.1f, 2.f, 10.f }) {
                Engine e; e.p[kOversampling] = (float)os; e.p[kStyle] = (float)st; e.p[kLookahead] = la; e.p[kAttack] = at; e.p[kGain] = 18; e.p[kCeiling] = -1; e.p[kRelease] = 30;
                e.setSampleRate(sr); std::vector<float> L, R; run(e, l, r, L, R);
                worst = std::max(worst, std::max(peakDb(L, 0), peakDb(R, 0)) + 1.f); ++cases;
            }
        }
        check(worst <= 0.001f, "ceiling -1 dB holds in %.0f cases with +18 dB gain (worst %+.4f dB over)", cases, worst); }
    // 2. exact latency, bit-transparent below the ceiling without oversampling
    for (int os = 0; os < 3; ++os) {
        Engine e; e.p[kOversampling] = (float)os; e.p[kLookahead] = 3; e.setSampleRate(fs);
        std::vector<float> in(fs, 0.f), L, R; for (int i = 100; i < fs; ++i) in[i] = 0.4f * std::sin(2 * kPi * 997 * i / fs);
        run(e, in, in, L, R); const int lat = (int)e.latency(); double err = 0, ref = 0;
        for (int i = fs / 2; i < fs; ++i) { err += (L[i] - in[i - lat]) * (L[i] - in[i - lat]); ref += in[i - lat] * in[i - lat]; }
        const double db = 10 * std::log10(err / ref + 1e-30);
        check(os == 0 ? db < -200 : db < -80, "oversampling %.0fx: latency %.0f samples, error vs delayed input %.0f dB", os == 0 ? 1.0 : (os == 1 ? 2.0 : 4.0), lat, db);
    }
    // 3. gain reduction on a steady tone: output sits at the ceiling, GR = gain + level - ceiling
    {   Engine e; e.p[kGain] = 10; e.p[kCeiling] = -3; e.setSampleRate(fs);
        std::vector<float> in(fs), L, R; for (int i = 0; i < fs; ++i) in[i] = 0.5f * std::sin(2 * kPi * 100 * i / fs);   // -6 dBFS
        run(e, in, in, L, R);
        check(std::fabs(peakDb(L, fs / 2) + 3) < 0.1 && std::fabs(e.meterGr() - 7) < 0.6, "steady -6 dBFS tone, +10 dB gain, -3 dB output: peak %.2f dB, GR %.1f dB (expect 7)", peakDb(L, fs / 2), e.meterGr()); }
    // 4. linking: a peak on the left only
    {   auto rightDip = [&](float link) { Engine e; e.p[kLinkTransient] = link; e.p[kLinkRelease] = link; e.p[kCeiling] = -6; e.setSampleRate(fs);
            std::vector<float> l(fs), r(fs), L, R; for (int i = 0; i < fs; ++i) { r[i] = 0.25f * std::sin(2 * kPi * 300 * i / fs); l[i] = r[i] * (i > fs / 4 && i < fs / 2 ? 4.f : 1.f); }
            run(e, l, r, L, R); float mn = 1; for (int i = fs / 4 + 2000; i < fs / 2 - 2000; i += 1) { (void)i; }
            double a = 0, b = 0; for (int i = fs / 4 + 4000; i < fs / 2 - 1000; ++i) { a += R[i] * R[i]; } for (int i = 2000; i < fs / 4 - 1000; ++i) b += R[i] * R[i];
            (void)mn; return 10 * std::log10((a / (fs / 4 - 5000)) / (b / (fs / 4 - 3000))); };
        const double linked = rightDip(1), free = rightDip(0);
        check(linked < -5 && std::fabs(free) < 0.2, "left-only peak: right channel moves %.1f dB linked, %.2f dB unlinked", linked, free); }
    // 5. release: time to recover 63% after a burst
    {   auto rec = [&](float ms) { Engine e; e.p[kRelease] = ms; e.p[kLookahead] = 1; e.p[kCeiling] = -6; e.p[kStyle] = kBalanced * 0 + 0; e.setSampleRate(fs);
            std::vector<float> in(fs), L, R; for (int i = 0; i < fs; ++i) in[i] = (i < fs / 4 ? 1.f : 0.1f) * std::sin(2 * kPi * 500 * i / fs);
            run(e, in, in, L, R, 64); int lat = e.latency();
            // envelope of output/input gain after the drop
            for (int i = fs / 4 + lat; i < fs; i += 48) { float pk = 0; for (int k = 0; k < 96; ++k) pk = std::max(pk, std::fabs(L[i + k])); if (gainToDb(pk / 0.1f) > -6 * 0.37f) return (i - fs / 4 - lat) * 1000.f / fs; }
            return 9999.f; };
        const float r20 = rec(20), r200 = rec(200);
        check(r200 > r20 * 4 && r20 < 60, "release 20 ms recovers in %.0f ms, 200 ms in %.0f ms", r20, r200); }
    // 6. true peaks: a fs/4 tone sampled at 45 degrees has inter-sample peaks 3 dB above its samples
    {   std::vector<float> in(fs), L, R; for (int i = 0; i < fs; ++i) in[i] = 0.7071f * std::sin(2 * kPi * 0.25f * i + kPi / 4);     // samples +-0.5, true peak 0.7071
        Engine m; m.p[kTruePeak] = 1; m.p[kCeiling] = 0; m.setSampleRate(fs); run(m, in, in, L, R);
        const float tp = m.meterOut(0);
        Engine s; s.p[kTruePeak] = 0; s.p[kCeiling] = 0; s.setSampleRate(fs); run(s, in, in, L, R);
        check(std::fabs(tp + 3.01f) < 0.3f && std::fabs(s.meterOut(0) + 6.02f) < 0.1f, "meters on an fs/4 tone: true peak %.2f dB (expect -3.0), sample peak %.2f dB (-6.0)", tp, s.meterOut(0));
        const std::vector<float> l = music(fs * 2, fs, 3);
        for (int os = 0; os < 3; ++os) { Engine e; e.p[kOversampling] = (float)os; e.p[kGain] = 16; e.p[kCeiling] = -1; e.setSampleRate(fs);
            std::vector<float> A, B; run(e, l, l, A, B); const float t = truePeakDb(A, fs / 2);
            check(os == 0 || t < -0.5f, "oversampling %.0fx, +16 dB into -1 dB: true peak %.2f dB", os == 0 ? 1.0 : (os == 1 ? 2.0 : 4.0), t); } }
    // 7. dither: quantised to the bit depth, noise at the expected level, shaping moves it upward
    {   auto noiseSpec = [&](int bits, int shaping, double& lowBand, double& highBand, bool& onGrid) {
            Engine e; e.p[kDither] = (float)bits; e.p[kShaping] = (float)shaping; e.p[kCeiling] = 0; e.setSampleRate(fs);
            std::vector<float> in(fs, 0.f), L, R; for (int i = 0; i < fs; ++i) in[i] = 0.001f * std::sin(2 * kPi * 1000 * i / fs);
            run(e, in, in, L, R);
            const float q = bits == 1 ? 32768.f : (bits == 2 ? 524288.f : 8388608.f); onGrid = true;
            for (int i = 1000; i < fs; ++i) { const float v = L[i] * q; if (std::fabs(v - std::round(v)) > 1e-3f) onGrid = false; }
            // band power of the error (output minus input) via simple DFT bins
            auto band = [&](float f0, float f1) { double s = 0; int nb = 0;
                for (float f = f0; f <= f1; f += 250) { double c = 0, d = 0; for (int i = 4000; i < 4000 + 8192; ++i) { const double er = L[i] - in[i]; c += er * std::cos(2 * kPi * f * i / fs); d += er * std::sin(2 * kPi * f * i / fs); } s += c * c + d * d; ++nb; }
                return 10 * std::log10(s / nb + 1e-30); };
            lowBand = band(1500, 4500); highBand = band(15000, 20000); };
        double l0, h0, l1, h1, l3, h3; bool g0, g1, g3;
        noiseSpec(1, 0, l0, h0, g0); noiseSpec(1, 1, l1, h1, g1); noiseSpec(1, 3, l3, h3, g3);
        check(g0 && g1 && g3, "16-bit dither output lies exactly on the 16-bit grid");
        check(std::fabs(h0 - l0) < 3, "flat dither is white (high band %+.1f dB vs 1.5-4.5 kHz)", h0 - l0);
        check(l1 < l0 - 3 && l3 < l0 - 8 && h3 > l3 + 15, "shaping lowers noise at 1.5-4.5 kHz: basic %.1f dB, weighted %.1f dB, weighted tilt %+.0f dB", l1 - l0, l3 - l0, h3 - l3);
        Engine off; off.setSampleRate(fs); std::vector<float> in(4800, 0.f), L, R; run(off, in, in, L, R); check(peakDb(L, 0) < -170, "dither off: silence stays digital silence"); }
    // 8. switching oversampling or look-ahead mid-stream: no overs, latency follows
    {   Engine e; e.p[kGain] = 12; e.p[kCeiling] = -1; e.setSampleRate(fs);
        const std::vector<float> l = music(fs * 2, fs, 5); std::vector<float> L(l.size()), R(l.size()); float worst = -100;
        for (size_t i = 0; i < l.size(); i += 256) {
            if (i == 256 * 40) e.p[kOversampling] = 2;
            if (i == 256 * 120) e.p[kLookahead] = 7;
            if (i == 256 * 200) { e.p[kOversampling] = 1; e.p[kLookahead] = 0.5f; }
            e.process(l.data() + i, l.data() + i, L.data() + i, R.data() + i, (uint32_t)std::min<size_t>(256, l.size() - i)); }
        worst = peakDb(L, 0);
        check(worst <= -1.0001f + 0.001f && e.latency() == (uint32_t)(32 + 24), "structural changes while playing: peak %.3f dB (ceiling -1), latency now %.0f", worst, e.latency()); }
    // 9. presets complete and in range
    {   int bad = 0; for (uint32_t p = 0; p < kNumPresets; ++p) for (uint32_t k = 0; k < kParamCount; ++k) { const ParamDef& d = paramDef(k);
            if (!(d.hints & kHintOutput) && (presets()[p].v[k] < d.min || presets()[p].v[k] > d.max)) ++bad; }
        check(bad == 0 && kNumPresets >= 24, "%.0f presets, %.0f values out of range", kNumPresets, bad); }
    printf(fails ? "%d FAILURES\n" : "ALL PASSED (0 failures)\n", fails); return fails;
}
