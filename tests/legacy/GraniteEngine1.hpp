// Granite — stereo look-ahead brickwall limiter engine.
// Clean-room implementation from public, generic techniques: per-sample required gain, a sliding
// minimum over the look-ahead window followed by a moving average (so gain changes are smooth yet
// never late), a one-pole release stage, separate channel linking for the transient and release
// stages, windowed-sinc halfband FIR oversampling (linear phase), TPDF dither with error-feedback
// noise shaping (coefficients after Wannamaker's 1992 weighted designs), and 4x interpolated true-peak
// metering. No third-party product code, assets or disassembly were used. License: MIT.

#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>

namespace granite1 {

static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float dbToGain(float db) { return std::pow(10.f, db / 20.f); }
inline float gainToDb(float g) { return 20.f * std::log10(std::max(g, 1e-9f)); }

// ------------------------------------------------------------------ params (append-only)
enum Param : uint32_t {
    kGain,          // dB, 0 .. 30: drive into the limiter
    kCeiling,       // dB, -30 .. 0: output level
    kStyle,         // Clear, Punchy, Dynamic, Balanced
    kLookahead,     // ms, 0 .. 10
    kAttack,        // ms, 0.1 .. 10 (limited to the look-ahead)
    kRelease,       // ms, 1 .. 2000
    kLinkTransient, // 0 .. 1
    kLinkRelease,   // 0 .. 1
    kOversampling,  // off, 2x, 4x
    kTruePeak,      // true-peak (inter-sample) metering
    kDither,        // off, 16, 20, 24 bit
    kShaping,       // flat, basic, optimized, weighted
    kMeterScale,    // dBFS, K-20, K-14, K-12 (display only)
    kDisplayRange,  // 12, 24, 48 dB (display only)
    kMidiOn,
    // outputs (read-only, for the UI)
    kOutLearn, kOutInL, kOutInR, kOutOutL, kOutOutR, kOutGr,
    kParamCount
};
enum Style { kClear, kPunchy, kDynamic, kBalanced, kNumStyles };

// ------------------------------------------------------------------ halfband oversampling
// 65-tap windowed-sinc halfband (Kaiser, beta 8): every other tap is zero, so each 2x stage
// costs 16 multiply-adds per output pair. Up + down = 32 samples of latency at the lower rate.
static constexpr int kHbHalf = 16, kHbC = 2 * kHbHalf;          // centre tap index 32 (65 taps)
inline const float* halfbandTaps() {                          // taps h[c + 2j + 1], j = 0..15
    static float t[kHbHalf];
    static bool done = false;
    if (!done) {
        done = true;
        auto bessel0 = [](double x) { double s = 1, term = 1; for (int k = 1; k < 30; ++k) { term *= (x / (2 * k)) * (x / (2 * k)); s += term; } return s; };
        const double beta = 8.0, N = 2 * kHbC; double sum = 0.5;
        for (int j = 0; j < kHbHalf; ++j) {
            const int k = 2 * j + 1;                                       // offset from the centre
            const double sinc = std::sin(3.14159265358979323846 * k / 2.0) / (3.14159265358979323846 * k / 2.0);
            const double r = (double)(k + kHbC) / N * 2 - 1;
            const double w = bessel0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / bessel0(beta);
            t[j] = (float)(0.5 * sinc * w); sum += 2 * t[j];
        }
        for (int j = 0; j < kHbHalf; ++j) t[j] = (float)(t[j] / sum);   // unity DC gain (centre 0.5/sum ~ 0.5)
    }
    return t;
}
// upsampler: one input sample -> two output samples (gain 2 so the level is unchanged)
struct HbUp {
    float buf[128] = {}; int pos = 0;                                 // ring of the last 64 inputs, stored twice
    void reset() { std::memset(buf, 0, sizeof buf); pos = 0; }
    inline void process(float x, float& y0, float& y1) {
        pos = (pos + 1) & 63; buf[pos] = buf[pos + 64] = x;
        const float* h = halfbandTaps(); const float* b = buf + pos + 64;   // b[-i] = x[n - i]
        y0 = b[-kHbHalf];                                                  // 2 * 0.5 * x[n - 16]
        float s = 0;
        for (int j = 0; j < kHbHalf; ++j) s += h[j] * (b[-kHbHalf - j] + b[-kHbHalf + j + 1]);
        y1 = 2.f * s;
    }
};
// downsampler: two input samples -> one output sample
struct HbDown {
    float buf[256] = {}; int pos = 0;                                 // ring of the last 128 inputs, stored twice
    void reset() { std::memset(buf, 0, sizeof buf); pos = 0; }
    inline float process(float x0, float x1) {
        pos = (pos + 1) & 127; buf[pos] = buf[pos + 128] = x0;
        pos = (pos + 1) & 127; buf[pos] = buf[pos + 128] = x1;
        const float* h = halfbandTaps(); const float* b = buf + pos + 128;  // b[-i] = v[m - i], m = 2n+1
        // output aligned to v[m - 1 - kHbC]... centre at m - kHbC - 1 (an even index)
        const float* c = b - kHbC - 1;
        float s = 0.5f * c[0];
        for (int j = 0; j < kHbHalf; ++j) s += h[j] * (c[-(2 * j + 1)] + c[2 * j + 1]);
        return s;
    }
};
// 1x, 2x or 4x oversampler for one channel; latency in base-rate samples: 0, 32, 48
struct Oversampler {
    HbUp u1, u2; HbDown d1, d2;
    void reset() { u1.reset(); u2.reset(); d1.reset(); d2.reset(); }
    static int latency(int os) { return os == 1 ? 0 : (os == 2 ? 32 : 48); }
    inline void up(float x, int os, float* y) {
        if (os == 1) { y[0] = x; return; }
        float a, b; u1.process(x, a, b);
        if (os == 2) { y[0] = a; y[1] = b; return; }
        u2.process(a, y[0], y[1]); u2.process(b, y[2], y[3]);      // one filter, fed in order
    }
    inline float down(const float* y, int os) {
        if (os == 1) return y[0];
        if (os == 2) return d1.process(y[0], y[1]);
        const float a = d2.process(y[0], y[1]), b = d2.process(y[2], y[3]);      // one filter, fed in order
        return d1.process(a, b);
    }
};

// ------------------------------------------------------------------ sliding minimum (monotonic deque)
struct SlidingMin {
    std::vector<float> val; std::vector<int64_t> idx; int head = 0, tail = 0, cap = 1;
    void init(int capacity) { cap = capacity + 2; val.assign(cap, 1.f); idx.assign(cap, 0); head = tail = 0; }
    void reset() { head = tail = 0; }
    // push value v at time t, drop entries older than t - window + 1, return the minimum
    inline float push(float v, int64_t t, int window) {
        while (tail != head) { const int last = (tail - 1 + cap) % cap; if (val[last] >= v) tail = last; else break; }
        val[tail] = v; idx[tail] = t; tail = (tail + 1) % cap;
        while (idx[head] <= t - window) head = (head + 1) % cap;
        return val[head];
    }
};

// ------------------------------------------------------------------ engine
class Engine {
public:
    float p[kParamCount];

    // level history for the display: one point per 20 ms, written by the audio thread
    static constexpr int kHist = 512;
    float hIn[kHist], hOut[kHist], hGr[kHist], hRms[kHist];
    struct AtomicIndex {          // copyable wrapper so the engine itself can be copied (tests)
        std::atomic<uint32_t> v { 0 };
        AtomicIndex() {}
        AtomicIndex(const AtomicIndex& o) : v(o.v.load()) {}
        AtomicIndex& operator=(const AtomicIndex& o) { v.store(o.v.load()); return *this; }
        uint32_t load() const { return v.load(std::memory_order_acquire); }
        void store(uint32_t x) { v.store(x, std::memory_order_release); }
    } histWrite;

    Engine() {
        std::fill(p, p + kParamCount, 0.f);
        p[kGain] = 0; p[kCeiling] = -0.1f; p[kStyle] = kClear; p[kLookahead] = 2; p[kAttack] = 2; p[kRelease] = 80;
        p[kLinkTransient] = 0.75f; p[kLinkRelease] = 1.f; p[kOversampling] = 0; p[kTruePeak] = 1;
        p[kDither] = 0; p[kShaping] = 2; p[kMeterScale] = 0; p[kDisplayRange] = 1; p[kMidiOn] = 1;
        for (int i = 0; i < kHist; ++i) { hIn[i] = hOut[i] = hRms[i] = -120; hGr[i] = 0; }
    }

    void setSampleRate(float sr) {
        fs = sr;
        const int maxL = (int)std::ceil(0.010 * sr) * 4 + 8;            // 10 ms at 4x
        for (int c = 0; c < 2; ++c) {
            delay[c].assign(maxL + 8, 0.f); boxRing[c].assign(maxL + 8, 1.f); mins[c].init(maxL + 8);
        }
        smooth = 1.f - std::exp(-1.f / (0.02f * sr));
        meterFall = std::exp(-1.f / (0.3f * sr));                       // peak meters fall back over ~300 ms
        rmsCoef = 1.f - std::exp(-1.f / (0.3f * sr));
        histSamples = std::max(1, (int)(0.02f * sr));
        configure(true);
        fadeGain = 1; fadeState = 0;
    }
    void reset() { configure(true); }

    // samples of delay the host must compensate for (base rate)
    uint32_t latency() const { return (uint32_t)(Oversampler::latency(os) + lookBase); }
    int oversampling() const { return os; }
    float meterIn(int c) const { return gainToDb(mIn[c]); }
    float meterOut(int c) const { return gainToDb(mOut[c]); }
    float meterGr() const { return mGr; }

    void process(const float* inL, const float* inR, float* outL, float* outR, uint32_t n) {
        // structural changes (oversampling, look-ahead length) switch over a short fade
        const int wantOs = osOf(p[kOversampling]), wantLook = lookOf(p[kLookahead]);
        if ((wantOs != os || wantLook != lookBase) && fadeState == 0) fadeState = 1;
        const int style = (int)clampf(std::round(p[kStyle]), 0, kNumStyles - 1);
        const float attackFactor = style == kPunchy ? 0.35f : (style == kDynamic ? 0.7f : 1.f);
        const int hi = (int)std::round(clampf(p[kAttack], 0.1f, 10.f) * 0.001f * fs * os * attackFactor);
        attackHi = std::max(1, std::min(hi, lookHi + 1));
        if (attackHi != lastAttack) refreshBox();
        const float relMs = clampf(p[kRelease], 1.f, 2000.f) * (style == kPunchy ? 0.5f : 1.f);
        const float lt = clampf(p[kLinkTransient], 0.f, 1.f), lr = clampf(p[kLinkRelease], 0.f, 1.f);
        const float tGain = dbToGain(clampf(p[kGain], -12.f, 30.f)), tCeil = dbToGain(clampf(p[kCeiling], -30.f, 0.f));
        const int bits = ditherBits(), shaping = (int)clampf(std::round(p[kShaping]), 0, 3);
        const bool truePeak = p[kTruePeak] > 0.5f;
        const float fsHi = fs * os, susCoef = 1.f - std::exp(-1.f / (0.3f * fsHi));

        for (uint32_t i = 0; i < n; ++i) {
            sGain += (tGain - sGain) * smooth; sCeil += (tCeil - sCeil) * smooth;
            const float x[2] = { inL[i] * sGain, inR[i] * sGain };
            float up[2][4], y[2][4];
            for (int c = 0; c < 2; ++c) { mIn[c] = std::max(mIn[c] * meterFall, std::fabs(x[c])); ovs[c].up(x[c], os, up[c]); }
            float grMin = 1.f;
            for (int k = 0; k < os; ++k) {
                // required gain per channel (Balanced softens the peaks first)
                float v[2], req[2];
                for (int c = 0; c < 2; ++c) {
                    v[c] = up[c][k];
                    if (style == kBalanced) v[c] = 0.6f * v[c] + 0.4f * soften(v[c], sCeil);
                    const float a = std::fabs(v[c]);
                    req[c] = a > sCeil ? sCeil / a : 1.f;
                }
                const float rmin = std::min(req[0], req[1]);
                float env[2];
                for (int c = 0; c < 2; ++c) {
                    const float r = req[c] + (rmin - req[c]) * lt;                       // transient link
                    const float m = mins[c].push(r, tHi, lookHi + 1);                   // worst gain needed ahead
                    lastMin[c] = m;
                    boxSum[c] += m - boxRing[c][boxPos];                                // moving average over the attack
                    boxRing[c][boxPos] = m;
                    env[c] = (float)(boxSum[c] / attackHi);
                    // release stage: follow down at once, recover at the release rate
                    float rel = relMs;
                    const float grDb = -gainToDb(rel_g[c]);
                    if (style == kClear) rel *= 1.f + 2.f * clampf(sus[c] / 4.f, 0.f, 1.f);     // sustained limiting: slower, less pumping
                    if (style == kDynamic) rel /= 1.f + grDb / 2.f;                            // deep, short peaks recover fast
                    const float rc = 1.f - std::exp(-1.f / (std::max(rel, 0.5f) * 0.001f * fsHi));
                    rel_g[c] = env[c] < rel_g[c] ? env[c] : rel_g[c] + (env[c] - rel_g[c]) * rc;
                    sus[c] += (grDb - sus[c]) * susCoef;
                }
                boxPos = (boxPos + 1) % attackHi;
                const float gmin = std::min(rel_g[0], rel_g[1]);
                for (int c = 0; c < 2; ++c) {
                    const float g = rel_g[c] + (gmin - rel_g[c]) * lr;                    // release link
                    // delayed signal (look-ahead)
                    std::vector<float>& d = delay[c];
                    d[dPos] = v[c];
                    const int rd = (dPos - lookHi + (int)d.size()) % (int)d.size();
                    y[c][k] = clampf(d[rd] * g, -sCeil, sCeil);                           // the clamp is a safety net only
                    grMin = std::min(grMin, g);
                }
                dPos = (dPos + 1) % (int)delay[0].size();
                ++tHi;
            }
            float o[2];
            for (int c = 0; c < 2; ++c) {
                o[c] = ovs[c].down(y[c], os);
                o[c] = clampf(o[c], -sCeil, sCeil);                                        // filter ripple stays under the ceiling
                if (bits > 0) {                     // dither, then keep the result on the grid and under the ceiling
                    const float q = std::ldexp(1.f, bits - 1), top = std::floor(sCeil * q) / q;
                    o[c] = clampf(dither(c, o[c], bits, shaping), -top, top);
                }
                o[c] *= fadeGain;
                const float pk = truePeak ? truePeakOf(c, o[c]) : std::fabs(o[c]);
                mOut[c] = std::max(mOut[c] * meterFall, pk);
                ms[c] += (o[c] * o[c] - ms[c]) * rmsCoef;
            }
            outL[i] = o[0]; outR[i] = o[1];
            const float grDb = -gainToDb(grMin);
            mGr = std::max(mGr * 0.9995f, grDb);
            // display history
            hPkIn = std::max(hPkIn, std::max(std::fabs(x[0]), std::fabs(x[1])));
            hPkOut = std::max(hPkOut, std::max(std::fabs(o[0]), std::fabs(o[1])));
            hGrMax = std::max(hGrMax, grDb);
            if (++hCount >= histSamples) {
                const uint32_t w = histWrite.load() % kHist;
                hIn[w] = gainToDb(hPkIn); hOut[w] = gainToDb(hPkOut); hGr[w] = hGrMax; hRms[w] = 10.f * std::log10(std::max(0.5f * (ms[0] + ms[1]), 1e-12f));
                histWrite.store(histWrite.load() + 1);
                hPkIn = hPkOut = 0; hGrMax = 0; hCount = 0;
            }
            // fade handling for structural changes
            if (fadeState == 1) { fadeGain -= 1.f / 128.f; if (fadeGain <= 0) { fadeGain = 0; configure(false); fadeState = 2; } }
            else if (fadeState == 2) { fadeGain += 1.f / 128.f; if (fadeGain >= 1) { fadeGain = 1; fadeState = 0; } }
        }
        for (int c = 0; c < 2; ++c) { if (!(std::fabs(ms[c]) < 1e6f)) ms[c] = 0; if (!(rel_g[c] >= 0 && rel_g[c] <= 1.0001f)) rel_g[c] = 1; }
    }

    // noise-shaping filters (error-feedback coefficients)
    static const float* shapingCoefs(int s, int& n) {
        static const float basic[1] = { 1.f };
        static const float optimized[3] = { 1.623f, -0.982f, 0.109f };                       // after Wannamaker, 3-tap E-weighted
        static const float weighted[9] = { 2.412f, -3.370f, 3.937f, -4.174f, 3.353f, -2.205f, 1.281f, -0.569f, 0.0847f };  // after Wannamaker, 9-tap F-weighted
        switch (s) { case 1: n = 1; return basic; case 2: n = 3; return optimized; case 3: n = 9; return weighted; default: n = 0; return nullptr; }
    }

private:
    static int osOf(float v) { const int k = (int)clampf(std::round(v), 0, 2); return k == 0 ? 1 : (k == 1 ? 2 : 4); }
    int lookOf(float ms) const { return (int)std::round(clampf(ms, 0.f, 10.f) * 0.001f * fs); }
    int ditherBits() const { const int k = (int)clampf(std::round(p[kDither]), 0, 3); return k == 0 ? 0 : (k == 1 ? 16 : (k == 2 ? 20 : 24)); }
    static float soften(float x, float ceil) {           // smooth saturation that approaches the ceiling
        const float k = 0.5f * ceil, a = std::fabs(x);
        if (a <= k) return x;
        return std::copysign(k + (ceil - k) * std::tanh((a - k) / (ceil - k)), x);
    }

    void configure(bool hard) {
        os = osOf(p[kOversampling]); lookBase = lookOf(p[kLookahead]); lookHi = lookBase * os;
        for (int c = 0; c < 2; ++c) {
            ovs[c].reset(); std::fill(delay[c].begin(), delay[c].end(), 0.f); mins[c].reset();
            rel_g[c] = 1; sus[c] = 0; for (float& e : err[c]) e = 0; tp[c].reset();
        }
        dPos = 0; tHi = 0; lastAttack = -1; attackHi = 1; lastMin[0] = lastMin[1] = 1.f;
        if (hard) { sGain = dbToGain(clampf(p[kGain], -12.f, 30.f)); sCeil = dbToGain(clampf(p[kCeiling], -30.f, 0.f));
            for (int c = 0; c < 2; ++c) { mIn[c] = mOut[c] = 0; ms[c] = 0; } mGr = 0; }
        refreshBox();
    }
    // new attack length: refill the moving average with the latest sliding minimum. That value covers
    // every sample still in the look-ahead delay, so the gain stays safe across the change.
    void refreshBox() {
        for (int c = 0; c < 2; ++c) { std::fill(boxRing[c].begin(), boxRing[c].end(), lastMin[c]); boxSum[c] = (double)lastMin[c] * attackHi; }
        boxPos = 0; lastAttack = attackHi;
    }
    float dither(int c, float x, int bits, int shaping) {
        const float q = std::ldexp(1.f, bits - 1);
        int n = 0; const float* h = shapingCoefs(shaping, n);
        float fb = 0; for (int i = 0; i < n; ++i) fb += h[i] * err[c][i];
        const float v = x * q - fb;
        rng = rng * 1664525u + 1013904223u; const float u1 = (rng >> 8) * (1.f / 16777216.f);
        rng = rng * 1664525u + 1013904223u; const float u2 = (rng >> 8) * (1.f / 16777216.f);
        const float yq = std::round(v + (u1 - u2));
        for (int i = 8; i > 0; --i) err[c][i] = err[c][i - 1];
        err[c][0] = clampf(yq - v, -4.f, 4.f);
        return yq / q;
    }
    struct TruePeak { HbUp a, b; void reset() { a.reset(); b.reset(); } };
    float truePeakOf(int c, float x) {
        float a0, a1, y[4]; tp[c].a.process(x, a0, a1); tp[c].b.process(a0, y[0], y[1]); tp[c].b.process(a1, y[2], y[3]);
        return std::max(std::max(std::fabs(y[0]), std::fabs(y[1])), std::max(std::fabs(y[2]), std::fabs(y[3])));
    }

    float fs = 48000.f, smooth = 0.01f, meterFall = 0.999f, rmsCoef = 0.0001f;
    int os = 1, lookBase = 0, lookHi = 0, attackHi = 1, lastAttack = -1, boxPos = 0, dPos = 0;
    int64_t tHi = 0;
    Oversampler ovs[2];
    std::vector<float> delay[2], boxRing[2];
    SlidingMin mins[2];
    double boxSum[2] = { 1, 1 };
    float rel_g[2] = { 1, 1 }, sus[2] = {}, sGain = 1, sCeil = 1, lastMin[2] = { 1, 1 };
    float err[2][9] = {}; uint32_t rng = 22222;
    TruePeak tp[2];
    float mIn[2] = {}, mOut[2] = {}, mGr = 0, ms[2] = {};
    float hPkIn = 0, hPkOut = 0, hGrMax = 0; int hCount = 0, histSamples = 960;
    float fadeGain = 1; int fadeState = 0;
};

} // namespace granite1
