// Parameter metadata shared by the DSP wrapper and the UI. License: MIT.
#pragma once
#include "GraniteEngine.hpp"
#include <cstdio>

namespace granite {

static constexpr uint32_t kHintInteger = 1, kHintBoolean = 2, kHintLog = 4, kHintOutput = 8, kHintNoAuto = 16, kHintView = 32;
struct ParamDef { const char* name; const char* sym; const char* unit; float min, max; uint32_t hints; };

inline const ParamDef& paramDef(uint32_t i) {
    static ParamDef t[kParamCount];
    static bool done = false;
    if (!done) {
        done = true;
        auto set = [](uint32_t i, const char* n, const char* s, const char* u, float lo, float hi, uint32_t h) { t[i] = { n, s, u, lo, hi, h }; };
        const uint32_t I = kHintInteger, B = kHintBoolean | kHintInteger, O = kHintOutput;
        set(kGain,          "Gain",            "gain",      "dB", 0, 30, 0);
        set(kCeiling,       "Output Level",    "ceiling",   "dB", -30, 0, 0);
        set(kStyle,         "Style",           "style",     "",   0, kNumStyles - 1, I);
        set(kLookahead,     "Lookahead",       "lookahead", "ms", 0, 10, 0);
        set(kAttack,        "Attack",          "attack",    "ms", 0.1f, 10, kHintLog);
        set(kRelease,       "Release",         "release",   "ms", 1, 2000, kHintLog);
        set(kLinkTransient, "Link Transients", "linktr",    "",   0, 1, 0);
        set(kLinkRelease,   "Link Release",    "linkrel",   "",   0, 1, 0);
        set(kOversampling,  "Oversampling",    "oversamp",  "",   0, kNumOversampling - 1, I);
        set(kTruePeak,      "True Peak Meters","truepeak",  "",   0, 1, B);
        set(kDither,        "Dither",          "dither",    "",   0, 3, I);
        set(kShaping,       "Noise Shaping",   "shaping",   "",   0, 3, I);
        set(kMeterScale,    "Meter Scale",     "meterscale","",   0, 3, I | kHintNoAuto);
        set(kDisplayRange,  "Display Range",   "disprange", "",   0, 2, I | kHintNoAuto);
        set(kMidiOn,        "MIDI",            "midion",    "",   0, 1, B);
        set(kOutLearn, "MIDI Learn",       "o_learn", "",   0, 10000000, O);
        set(kOutInL,   "Input Level L",    "o_inl",   "dB", -120, 40, O);
        set(kOutInR,   "Input Level R",    "o_inr",   "dB", -120, 40, O);
        set(kOutOutL,  "Output Level L",   "o_outl",  "dB", -120, 12, O);
        set(kOutOutR,  "Output Level R",   "o_outr",  "dB", -120, 12, O);
        set(kOutGr,    "Gain Reduction",   "o_gr",    "dB", 0, 60, O);
        const uint32_t V = kHintView;
        set(kTruePeakLimit, "True Peak Limiting", "tplimit", "", 0, 1, B);
        set(kSidechain,     "Sidechain",          "sidechain", "", 0, 1, I);
        set(kDcFilter,      "DC Filter",          "dcfilter", "", 0, 1, B);
        set(kUnityGain,     "Unity Gain",         "unity",    "", 0, 1, B | V);
        set(kAudition,      "Audition Limiting",  "audition", "", 0, 1, B | V);
        set(kLoudTarget,    "Loudness Target",    "loudtarget", "", 0, kNumLoudTargets - 1, I | V | kHintNoAuto);
        set(kDisplaySpeed,  "Display Speed",      "dispspeed", "", 0, 3, I | V | kHintNoAuto);
        set(kCompact,       "Compact View",       "compact",  "", 0, 1, B | V | kHintNoAuto);
        set(kOutLoudM,       "Momentary Loudness",  "o_loudm", "LUFS", -144, 10, O);
        set(kOutLoudS,       "Short-term Loudness", "o_louds", "LUFS", -144, 10, O);
        set(kOutLoudI,       "Integrated Loudness", "o_loudi", "LUFS", -144, 10, O);
        set(kOutLoudRange,   "Loudness Range",      "o_lra",   "LU", 0, 60, O);
        set(kOutTruePeakMax, "True Peak Max",       "o_tpmax", "dBTP", -144, 24, O);
    }
    return t[i];
}

inline const char* const* enumLabels(uint32_t p, int& count) {
    static const char* const style[] = { "Clear", "Punchy", "Dynamic", "Balanced", "Aggressive", "Modern", "Bus", "Safe" };
    static const char* const os[] = { "Off", "2x", "4x", "8x", "16x", "32x" };
    static const char* const sc[] = { "Internal", "External" };
    static const char* const tgt[] = { "Off", "EBU R128 (-23 LUFS)", "ATSC A/85 (-24 LKFS)", "Streaming (-14 LUFS)", "Podcast (-16 LUFS)" };
    static const char* const spd[] = { "Normal", "Fast", "Slow", "Off" };
    static const char* const dith[] = { "Off", "16 bit", "20 bit", "24 bit" };
    static const char* const shp[] = { "Flat", "Basic", "Optimized", "Weighted" };
    static const char* const scale[] = { "dBFS", "K-20", "K-14", "K-12" };
    static const char* const range[] = { "12 dB", "24 dB", "48 dB" };
    switch (p) {
    case kStyle: count = kNumStyles; return style;
    case kOversampling: count = kNumOversampling; return os;
    case kSidechain: count = 2; return sc;
    case kLoudTarget: count = kNumLoudTargets; return tgt;
    case kDisplaySpeed: count = 4; return spd;
    case kDither: count = 4; return dith;
    case kShaping: count = 4; return shp;
    case kMeterScale: count = 4; return scale;
    case kDisplayRange: count = 3; return range;
    default: count = 0; return nullptr;
    }
}

inline const char* enumLabelOf(uint32_t p, int k) { int n = 0; const char* const* l = enumLabels(p, n); return l && n > 0 ? l[std::max(0, std::min(n - 1, k))] : ""; }

// Normalised 0..1 <-> value (log where marked); used by the UI and MIDI learn.
inline float toNorm(uint32_t p, float v) {
    const ParamDef& d = paramDef(p);
    if (d.hints & kHintLog) return std::log(v / d.min) / std::log(d.max / d.min);
    return (v - d.min) / (d.max - d.min);
}
inline float fromNorm(uint32_t p, float n) {
    const ParamDef& d = paramDef(p);
    n = clampf(n, 0.f, 1.f);
    float v = (d.hints & kHintLog) ? d.min * std::pow(d.max / d.min, n) : d.min + n * (d.max - d.min);
    if (d.hints & kHintInteger) v = std::round(v);
    return v;
}
// settings: saved in presets, A/B and undo (not meters, not view-only switches)
inline bool isSetting(uint32_t p) { return !(paramDef(p).hints & (kHintOutput | kHintView)); }
inline float displayRangeDb(float v) { const int k = (int)clampf(std::round(v), 0, 2); return k == 0 ? 12.f : (k == 1 ? 24.f : 48.f); }
inline float kOffsetDb(float v) { const int k = (int)clampf(std::round(v), 0, 3); return k == 0 ? 0.f : (k == 1 ? 20.f : (k == 2 ? 14.f : 12.f)); }

} // namespace granite
