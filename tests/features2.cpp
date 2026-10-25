// Granite 2.0 behaviour: the four new styles, oversampling to 32x, true-peak limiting, external sidechain,
// unity gain and audition monitoring, the DC filter, loudness metering, latency of every configuration.
#include "../plugins/Granite/ParamDefs.hpp"
#include "../plugins/Granite/Presets.hpp"
#include <cstdio>
#include <vector>
using namespace granite;

static int fails = 0;
#define CHECK(c, ...) do { bool ok_ = (c); char m_[300]; snprintf(m_, sizeof m_, __VA_ARGS__); printf("%s  %s\n", ok_ ? "PASS" : "FAIL", m_); if (!ok_) fails++; } while (0)

static void run(Engine& e, const std::vector<float>& l, const std::vector<float>& r, std::vector<float>& L, std::vector<float>& R,
                const std::vector<float>* sl = nullptr, const std::vector<float>* sr = nullptr, int block = 256) {
    L.resize(l.size()); R.resize(l.size());
    for (size_t i = 0; i < l.size(); i += block) {
        const uint32_t n = (uint32_t)std::min<size_t>(block, l.size() - i);
        e.process(l.data() + i, r.data() + i, L.data() + i, R.data() + i, n, sl ? sl->data() + i : nullptr, sr ? sr->data() + i : nullptr);
    }
}
static std::vector<float> music(int n, float fs, uint32_t seed = 3) {     // drums-like bursts + bass + noise
    std::vector<float> v(n); uint32_t r = seed; float lp = 0;
    for (int i = 0; i < n; ++i) { r = r * 1664525u + 1013904223u; const float w = (int32_t)r * (1.f / 2147483648.f); lp += 0.08f * (w - lp);
        const float t = std::fmod(i / fs, 0.5f), hit = std::exp(-t * 30.f);
        v[i] = 0.5f * hit * w + 0.35f * std::sin(2 * kPi * 55 * i / fs) + 0.15f * lp * 4.f + 0.6f * hit * std::sin(2 * kPi * 80 * i / fs * (1 + 2 * hit)); }
    return v;
}
static float peakDb(const std::vector<float>& v, size_t a) { float p = 0; for (size_t i = a; i < v.size(); ++i) p = std::max(p, std::fabs(v[i])); return gainToDb(p); }
static float rmsDb(const std::vector<float>& v, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += (double)v[i] * v[i]; return 10.f * (float)std::log10(s / (b - a) + 1e-30); }
static float truePeakDb(const std::vector<float>& v, size_t a) {    // 8x windowed-sinc interpolation, independent of the engine
    float p = 0;
    for (size_t i = a + 32; i + 33 < v.size(); ++i) for (int ph = 0; ph < 8; ++ph) {
        const double t = ph / 8.0; double s = 0;
        for (int k = -31; k <= 32; ++k) { const double x = k - t; const double sinc = std::fabs(x) < 1e-9 ? 1 : std::sin(3.14159265358979 * x) / (3.14159265358979 * x);
            const double w = 0.42 + 0.5 * std::cos(3.14159265358979 * x / 33) + 0.08 * std::cos(2 * 3.14159265358979 * x / 33); s += v[i + k] * sinc * w; }
        p = std::max(p, (float)std::fabs(s)); }
    return gainToDb(p);
}

int main() {
    const int fs = 48000;
    const std::vector<float> ml = music(fs * 2, (float)fs, 3), mr = music(fs * 2, (float)fs, 9);
    const std::vector<float> sl(ml.begin(), ml.begin() + fs), sr(mr.begin(), mr.begin() + fs);     // 1 s, for the heavy sweeps

    // 1. the ceiling holds for all eight styles at every oversampling factor, with and without true-peak limiting
    {   float worst = -100; int cases = 0;
        for (int st = 0; st < kNumStyles; ++st) for (int os = 0; os < kNumOversampling; ++os) for (int tpl = 0; tpl < 2; ++tpl) {
            Engine e; e.p[kStyle] = (float)st; e.p[kOversampling] = (float)os; e.p[kTruePeakLimit] = (float)tpl; e.p[kGain] = 18; e.p[kCeiling] = -1;
            e.p[kLookahead] = 1.5f; e.p[kAttack] = 1; e.p[kRelease] = 30; e.setSampleRate((float)fs);
            std::vector<float> L, R; run(e, sl, sr, L, R); worst = std::max(worst, std::max(peakDb(L, 0), peakDb(R, 0)) + 1.f); ++cases; }
        CHECK(worst <= 0.001f, "sample-peak ceiling holds: %d cases (8 styles x 6 oversampling x TP limiting), worst %+.4f dB over", cases, worst); }

    // 2. true-peak limiting keeps inter-sample peaks under the ceiling (measured by an independent 8x interpolator)
    {   float worstOn = -100, worstOff = -100;
        for (int st = 0; st < kNumStyles; ++st) for (int os : { 0, 1, 2, 3 }) for (int tpl = 0; tpl < 2; ++tpl) {
            Engine e; e.p[kStyle] = (float)st; e.p[kOversampling] = (float)os; e.p[kTruePeakLimit] = (float)tpl; e.p[kGain] = 16; e.p[kCeiling] = -1;
            e.setSampleRate((float)fs);
            std::vector<float> L, R; run(e, sl, sr, L, R);
            const float t = std::max(truePeakDb(L, fs / 4), truePeakDb(R, fs / 4)) + 1.f;
            (tpl ? worstOn : worstOff) = std::max(tpl ? worstOn : worstOff, t); }
        CHECK(worstOn <= 0.05f && worstOff > 0.3f, "true peak vs ceiling, 8 styles x 1-8x, +16 dB: worst %+.2f dB with TP limiting (%+.2f dB without)", worstOn, worstOff);
        std::vector<float> in(fs), L, R; for (int i = 0; i < fs; ++i) in[i] = 0.99f * std::sin(2 * kPi * 0.25f * i + kPi / 4);      // fs/4 at 45 degrees: true peak 3 dB over the samples
        Engine e; e.p[kTruePeakLimit] = 1; e.p[kCeiling] = -1; e.p[kGain] = 3; e.setSampleRate((float)fs); run(e, in, in, L, R);
        CHECK(truePeakDb(L, fs / 2) <= -0.95f && peakDb(L, fs / 2) < -3.5f, "fs/4 tone with 3 dB inter-sample overs: true peak %.2f dB, sample peak %.2f dB (ceiling -1)", truePeakDb(L, fs / 2), peakDb(L, fs / 2)); }

    // 3. latency is exact for every oversampling factor, with and without the true-peak stage
    {   int bad = 0; float worst = -300;
        for (int os = 0; os < kNumOversampling; ++os) for (int tpl = 0; tpl < 2; ++tpl) {
            Engine e; e.p[kOversampling] = (float)os; e.p[kTruePeakLimit] = (float)tpl; e.p[kLookahead] = 3; e.setSampleRate((float)fs);
            std::vector<float> in(fs, 0.f), L, R; for (int i = 100; i < fs; ++i) in[i] = 0.4f * std::sin(2 * kPi * 997 * i / fs);
            run(e, in, in, L, R); const int lat = (int)e.latency(); double err = 0, ref = 0;
            for (int i = fs / 2; i < fs; ++i) { err += (L[i] - in[i - lat]) * (L[i] - in[i - lat]); ref += in[i - lat] * in[i - lat]; }
            const float db = (float)(10 * std::log10(err / ref + 1e-30)); worst = std::max(worst, db);
            const int expect = OversamplerN::latency(1 << os) + 144 + (tpl ? TruePeakStage::kInterpDelay + 24 : 0);
            if (lat != expect || db > -80) { ++bad; printf("   os %d tp %d: latency %d (expect %d), error %.1f dB\n", 1 << os, tpl, lat, expect, db); } }
        CHECK(bad == 0, "latency exact for 1x-32x with/without true-peak limiting; worst error vs delayed input %.0f dB", worst); }

    // 4. the new styles have their own character
    {   auto render = [&](int st, float& rms, float& removed) { Engine e; e.p[kStyle] = (float)st; e.p[kGain] = 14; e.p[kCeiling] = -1; e.p[kRelease] = 60; e.setSampleRate((float)fs);
            std::vector<float> L, R; run(e, ml, mr, L, R); rms = rmsDb(L, fs / 2, ml.size()); e.p[kGain] = 14; return L; };
        float rClear, rAgg, rSafe, rBus, rMod, d; render(kClear, rClear, d); render(kAggressive, rAgg, d); render(kSafe, rSafe, d); render(kBus, rBus, d); render(kModern, rMod, d);
        CHECK(rAgg > rClear + 0.5f && rSafe < rClear && rBus < rClear && rMod > rSafe,
              "loudness at the same settings: Aggressive %.2f > Clear %.2f > Safe %.2f dB RMS; Bus %.2f, Modern %.2f", rAgg, rClear, rSafe, rBus, rMod);
        // distortion on a pure bass tone pushed 10 dB over: Safe keeps it lowest, Aggressive highest
        auto thd = [&](int st) { Engine e; e.p[kStyle] = (float)st; e.p[kGain] = 10; e.p[kCeiling] = -1; e.p[kRelease] = 10; e.p[kLookahead] = 1; e.p[kAttack] = 1; e.setSampleRate((float)fs);
            std::vector<float> in(fs), L, R; for (int i = 0; i < fs; ++i) in[i] = 0.5f * std::sin(2 * kPi * 60 * i / fs); run(e, in, in, L, R);
            const int a = fs / 2, n = 24000; double f1c = 0, f1s = 0, tot = 0;      // 60 Hz fits 30 cycles in 0.5 s
            for (int i = a; i < a + n; ++i) { f1c += L[i] * std::cos(2 * kPi * 60 * i / fs); f1s += L[i] * std::sin(2 * kPi * 60 * i / fs); tot += L[i] * L[i]; }
            const double fund = 2 * (f1c * f1c + f1s * f1s) / n; return (float)(10 * std::log10(std::max(1e-12, tot - fund) / fund)); };
        const float tSafe = thd(kSafe), tClear = thd(kClear), tAgg = thd(kAggressive);
        CHECK(tSafe < tClear && tClear < tAgg, "distortion on a 60 Hz tone 10 dB over (release 10 ms): Safe %.1f dB < Clear %.1f dB < Aggressive %.1f dB", tSafe, tClear, tAgg);
        // sustained limiting then a drop: Bus and Safe recover more slowly than Punchy; Modern recovers faster after a lone peak than Clear
        auto recover = [&](int st, bool sustained) { Engine e; e.p[kStyle] = (float)st; e.p[kCeiling] = -6; e.p[kRelease] = 80; e.p[kLookahead] = 1; e.setSampleRate((float)fs);
            std::vector<float> in(fs * 2), L, R; const int drop = sustained ? fs : fs / 2 + 480;
            for (int i = 0; i < fs * 2; ++i) in[i] = (i < drop && (sustained || i >= fs / 2) ? 1.f : 0.1f) * std::sin(2 * kPi * 500 * i / fs);
            run(e, in, in, L, R, nullptr, nullptr, 64); const int lat = (int)e.latency();
            for (int i = drop + lat; i < fs * 2 - 200; i += 48) { float pk = 0; for (int k = 0; k < 96; ++k) pk = std::max(pk, std::fabs(L[i + k])); if (gainToDb(pk / 0.1f) > -1.f) return (i - drop - lat) * 1000.f / fs; }
            return 9999.f; };
        const float pS = recover(kPunchy, true), bS = recover(kBus, true), sS = recover(kSafe, true), cP = recover(kClear, false), mP = recover(kModern, false);
        CHECK(bS > pS * 2 && sS > pS * 2 && mP < cP, "recovery to -1 dB: after 0.5 s of limiting Punchy %.0f ms, Bus %.0f ms, Safe %.0f ms; after a 10 ms peak Clear %.0f ms, Modern %.0f ms", pS, bS, sS, cP, mP); }

    // 5. external sidechain: the key decides the gain, the audio follows it
    {   std::vector<float> a(fs), k(fs), z(fs, 0.f), L, R;
        for (int i = 0; i < fs; ++i) { a[i] = 0.2f * std::sin(2 * kPi * 220 * i / fs); k[i] = (i > fs / 2 ? 2.f : 0.1f) * std::sin(2 * kPi * 90 * i / fs); }
        Engine e; e.p[kSidechain] = 1; e.p[kCeiling] = -6; e.p[kRelease] = 50; e.setSampleRate((float)fs); run(e, a, a, L, R, &k, &k);
        const float early = rmsDb(L, fs / 8, fs / 2 - 1000) - rmsDb(a, fs / 8, fs / 2 - 1000), late = rmsDb(L, fs * 3 / 4, fs) - rmsDb(a, fs * 3 / 4, fs);
        Engine in; in.p[kCeiling] = -6; in.setSampleRate((float)fs); run(in, a, a, L, R, &k, &k);
        const float internal = rmsDb(L, fs * 3 / 4, fs) - rmsDb(a, fs * 3 / 4, fs);
        std::vector<float> loud(fs); for (int i = 0; i < fs; ++i) loud[i] = 0.9f * std::sin(2 * kPi * 220 * i / fs);
        Engine q; q.p[kSidechain] = 1; q.p[kCeiling] = -6; q.setSampleRate((float)fs); run(q, loud, loud, L, R, &z, &z);
        CHECK(std::fabs(early) < 0.05f && std::fabs(late + 12.0f) < 0.6f && std::fabs(internal) < 0.05f && peakDb(L, fs / 2) > -1.f,
              "external key at +6 dBFS over a -6 dB ceiling ducks the audio %.1f dB (expect -12), quiet key %.2f dB; internal ignores it (%.2f dB); silent key leaves a loud input alone (peak %.1f dB)",
              late, early, internal, peakDb(L, fs / 2)); }

    // 6. unity gain and audition limiting
    {   std::vector<float> quiet(fs), L, R, L2, R2; for (int i = 0; i < fs; ++i) quiet[i] = 0.05f * std::sin(2 * kPi * 440 * i / fs);
        Engine u; u.p[kGain] = 12; u.p[kUnityGain] = 1; u.setSampleRate((float)fs); run(u, quiet, quiet, L, R);
        const int lat = (int)u.latency(); double err = 0; for (int i = fs / 2; i < fs; ++i) err = std::max(err, (double)std::fabs(L[i] - quiet[i - lat]));
        CHECK(err < 1e-5, "unity gain: +12 dB into an unlimited signal comes back at the input level (max error %.1e)", err);
        Engine a; a.p[kGain] = 12; a.p[kAudition] = 1; a.setSampleRate((float)fs); run(a, quiet, quiet, L, R);
        CHECK(peakDb(L, fs / 2) < -100, "audition with nothing to limit is silent (%.0f dB)", peakDb(L, fs / 2));
        Engine n; n.p[kGain] = 12; n.p[kCeiling] = -1; n.setSampleRate((float)fs); run(n, ml, mr, L, R);
        Engine d; d.p[kGain] = 12; d.p[kCeiling] = -1; d.p[kAudition] = 1; d.setSampleRate((float)fs); run(d, ml, mr, L2, R2);
        const int l2 = (int)d.latency(); double e2 = 0; const float g = dbToGain(12);
        for (int i = fs / 2; i < (int)ml.size(); ++i) e2 = std::max(e2, (double)std::fabs(L[i] + L2[i] - g * ml[i - l2]));
        const float rem = rmsDb(L2, fs / 2, ml.size()) - rmsDb(L, fs / 2, ml.size());
        Engine du; du.p[kGain] = 12; du.p[kCeiling] = -1; du.p[kAudition] = 1; du.p[kUnityGain] = 1; du.setSampleRate((float)fs); run(du, ml, mr, L, R);
        const float unityRem = rmsDb(L, fs / 2, ml.size()) - rmsDb(L2, fs / 2, ml.size());
        CHECK(e2 < 1e-3 && std::fabs(rem) < 20 && std::fabs(unityRem + 12) < 0.01f, "audition: output + removed part = the delayed input (max error %.1e); removed part %+.1f dB vs the output; with unity gain %.2f dB", e2, rem, unityRem); }

    // 7. DC filter
    {   std::vector<float> in(fs * 2), L, R; for (int i = 0; i < fs * 2; ++i) in[i] = 0.3f + 0.2f * std::sin(2 * kPi * 200 * i / fs);
        auto meanOf = [&](const std::vector<float>& v) { double s = 0; for (int i = fs; i < fs * 2; ++i) s += v[i]; return s / fs; };
        Engine on; on.p[kDcFilter] = 1; on.setSampleRate((float)fs); run(on, in, in, L, R); const double m1 = meanOf(L), lvl = rmsDb(L, fs, fs * 2);
        Engine off; off.setSampleRate((float)fs); run(off, in, in, L, R); const double m0 = meanOf(L);
        CHECK(std::fabs(m1) < 1e-3 && std::fabs(m0 - 0.3) < 1e-3 && std::fabs(lvl - rmsDb(in, fs, fs * 2) + 10 * std::log10(1 + 0.3 * 0.3 / 0.02)) < 0.1,
              "DC filter: a +0.3 offset becomes %.5f (off: %.4f); the 200 Hz tone keeps its level", m1, m0); }

    // 8. loudness metering of the output
    {   std::vector<float> in(fs * 5), L, R; const float a = dbToGain(-23); for (int i = 0; i < fs * 5; ++i) in[i] = a * std::sin(2 * kPi * 1000 * i / fs);
        Engine e; e.p[kCeiling] = 0; e.setSampleRate((float)fs); run(e, in, in, L, R);
        CHECK(std::fabs(e.loudIntegrated() + 23) < 0.1f && std::fabs(e.loudMomentary() + 23) < 0.1f && std::fabs(e.loudShortTerm() + 23) < 0.1f && std::fabs(e.loudTruePeak() + 23) < 0.2f,
              "loudness: 1 kHz at -23 dBFS reads %.2f LUFS integrated, %.2f momentary, %.2f short-term, %.2f dBTP", e.loudIntegrated(), e.loudMomentary(), e.loudShortTerm(), e.loudTruePeak());
        e.p[kGain] = 10; e.p[kCeiling] = -1; run(e, ml, mr, L, R);
        { std::vector<float> lead(in.begin(), in.begin() + fs / 2); run(e, lead, lead, L, R); }      // flush the latency
        e.resetLoudness(); run(e, in, in, L, R);
        CHECK(std::fabs(e.loudIntegrated() + 13) < 0.1f && e.loudTruePeak() < -12.8f, "after a reset only the new material counts: %.2f LUFS (expect -13), %.2f dBTP", e.loudIntegrated(), e.loudTruePeak());
        CHECK(std::fabs(loudTargetLufs(1) + 23) < 1e-6f && std::fabs(loudTargetLufs(2) + 24) < 1e-6f && std::fabs(loudTargetLufs(3) + 14) < 1e-6f, "loudness targets: EBU R128 -23, ATSC A/85 -24, streaming -14 LUFS"); }

    // 9. switching to 32x, true-peak limiting and back while playing: no overs, latency follows
    {   Engine e; e.p[kGain] = 12; e.p[kCeiling] = -1; e.setSampleRate((float)fs);
        std::vector<float> L(ml.size()), R(ml.size());
        for (size_t i = 0; i < ml.size(); i += 256) {
            if (i == 256 * 40) e.p[kOversampling] = 5;
            if (i == 256 * 120) e.p[kTruePeakLimit] = 1;
            if (i == 256 * 200) { e.p[kOversampling] = 3; e.p[kStyle] = kModern; }
            e.process(ml.data() + i, mr.data() + i, L.data() + i, R.data() + i, (uint32_t)std::min<size_t>(256, ml.size() - i)); }
        CHECK(peakDb(L, 0) <= -0.999f && e.latency() == (uint32_t)(56 + 96 + 48), "switching 32x, true-peak limiting, 8x while playing: peak %.3f dB, latency %u", peakDb(L, 0), e.latency()); }

    // 10. presets: complete, in range, view switches left alone
    {   int bad = 0, view = 0; for (uint32_t p = 0; p < kNumPresets; ++p) for (uint32_t k = 0; k < kParamCount; ++k) { const ParamDef& d = paramDef(k);
            if (!(d.hints & kHintOutput) && (presets()[p].v[k] < d.min || presets()[p].v[k] > d.max)) ++bad;
            if ((d.hints & kHintView) && presets()[p].v[k] != Engine().p[k]) ++view; }
        CHECK(bad == 0 && view == 0 && kNumPresets >= 37, "%u presets, %d values out of range, %d view switches set", kNumPresets, bad, view); }

    printf("%s (%d failures)\n", fails ? "FAILED" : "ALL PASSED", fails);
    return fails;
}
