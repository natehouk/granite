// Factory presets (each lists only what it changes from the engine defaults). License: MIT.
// Engine defaults: Clear, gain 0 dB, output -0.1 dB, look-ahead 2 ms, attack 2 ms, release 80 ms,
// transient link 75%, release link 100%, no oversampling, true-peak meters, no dither;
// 2.0: no true-peak limiting, internal sidechain, no DC filter.
#pragma once
#include "GraniteEngine.hpp"
#include <algorithm>

namespace granite {

struct PV { uint32_t id; float v; };
struct PresetDef { const char* name; const char* category; const PV* pv; };
struct Preset { const char* name; const char* category; float v[kParamCount]; };

static const char* const kCategories[] = { "Mastering", "Genres", "Mix & Bus", "Utility" };
static constexpr int kNumCategories = 4;

#define END { kParamCount, 0 }
namespace pv {
static const PV init[] = { END };
// ---------------------------------------------------------------- Mastering
static const PV transparent[] = { {kGain,4}, {kCeiling,-1}, {kLookahead,5}, {kAttack,5}, {kRelease,150}, {kOversampling,2}, END };
static const PV loudClean[]   = { {kGain,8}, {kCeiling,-1}, {kLookahead,4}, {kAttack,4}, {kRelease,120}, {kOversampling,2}, END };
static const PV streaming[]   = { {kGain,5}, {kCeiling,-1}, {kLookahead,5}, {kAttack,4}, {kRelease,200}, {kOversampling,2}, {kStyle,kBalanced}, END };
static const PV cd16[]        = { {kGain,6}, {kCeiling,-0.3f}, {kLookahead,4}, {kAttack,4}, {kRelease,100}, {kOversampling,2}, {kDither,1}, {kShaping,3}, END };
static const PV gentle[]      = { {kGain,2}, {kCeiling,-1}, {kLookahead,6}, {kAttack,6}, {kRelease,300}, {kOversampling,1}, END };
static const PV vinylPre[]    = { {kGain,3}, {kCeiling,-3}, {kLookahead,6}, {kAttack,6}, {kRelease,400}, {kOversampling,2}, {kStyle,kBalanced}, END };
// ---------------------------------------------------------------- Genres
static const PV edm[]      = { {kGain,10}, {kCeiling,-0.5f}, {kStyle,kPunchy}, {kLookahead,1.5f}, {kAttack,1}, {kRelease,40}, {kOversampling,2}, END };
static const PV hiphop[]   = { {kGain,8}, {kCeiling,-0.5f}, {kStyle,kPunchy}, {kLookahead,2}, {kAttack,2}, {kRelease,70}, {kOversampling,1}, END };
static const PV rock[]     = { {kGain,7}, {kCeiling,-0.5f}, {kStyle,kBalanced}, {kLookahead,3}, {kAttack,3}, {kRelease,90}, {kOversampling,1}, END };
static const PV pop[]      = { {kGain,7}, {kCeiling,-1}, {kStyle,kClear}, {kLookahead,4}, {kAttack,3}, {kRelease,100}, {kOversampling,2}, END };
static const PV acoustic[] = { {kGain,3}, {kCeiling,-1}, {kStyle,kDynamic}, {kLookahead,5}, {kAttack,5}, {kRelease,250}, {kOversampling,2}, END };
static const PV jazz[]     = { {kGain,2}, {kCeiling,-1}, {kStyle,kDynamic}, {kLookahead,6}, {kAttack,6}, {kRelease,350}, {kOversampling,2}, END };
static const PV metal[]    = { {kGain,9}, {kCeiling,-0.3f}, {kStyle,kBalanced}, {kLookahead,2}, {kAttack,1.5f}, {kRelease,50}, {kOversampling,2}, END };
static const PV ambient[]  = { {kGain,3}, {kCeiling,-1}, {kStyle,kClear}, {kLookahead,8}, {kAttack,8}, {kRelease,800}, {kOversampling,2}, END };
// ---------------------------------------------------------------- Mix & Bus
static const PV drumBus[]   = { {kGain,6}, {kStyle,kPunchy}, {kLookahead,1}, {kAttack,0.5f}, {kRelease,30}, {kLinkTransient,0.5f}, {kLinkRelease,0.5f}, END };
static const PV snarePeak[] = { {kGain,5}, {kStyle,kPunchy}, {kLookahead,0.5f}, {kAttack,0.3f}, {kRelease,20}, END };
static const PV vocalCatch[] = { {kGain,3}, {kCeiling,-3}, {kStyle,kClear}, {kLookahead,3}, {kAttack,2}, {kRelease,60}, END };
static const PV bassLevel[] = { {kGain,6}, {kCeiling,-2}, {kStyle,kDynamic}, {kLookahead,5}, {kAttack,5}, {kRelease,150}, {kLinkTransient,1}, END };
static const PV wideBus[]   = { {kGain,4}, {kCeiling,-1}, {kLinkTransient,0}, {kLinkRelease,0.3f}, {kLookahead,3}, {kAttack,3}, {kRelease,120}, END };
static const PV pumping[]   = { {kGain,14}, {kCeiling,-1}, {kStyle,kDynamic}, {kLookahead,0.5f}, {kAttack,0.5f}, {kRelease,250}, END };
// ---------------------------------------------------------------- Utility
static const PV safety[]   = { {kCeiling,-0.3f}, {kLookahead,1}, {kAttack,1}, {kRelease,50}, END };
static const PV broadcast[] = { {kGain,4}, {kCeiling,-2}, {kLookahead,5}, {kAttack,5}, {kRelease,200}, {kOversampling,2}, {kStyle,kBalanced}, END };
static const PV clipper[]  = { {kGain,6}, {kCeiling,-1}, {kLookahead,0}, {kAttack,0.1f}, {kRelease,1}, {kStyle,kBalanced}, END };
static const PV dither16[] = { {kDither,1}, {kShaping,3}, {kCeiling,-0.1f}, END };
static const PV dither24[] = { {kDither,3}, {kShaping,0}, {kCeiling,-0.1f}, END };
static const PV kMeter14[] = { {kMeterScale,2}, {kDisplayRange,2}, {kCeiling,-1}, END };
// ---------------------------------------------------------------- 2.0
static const PV streamTp[]   = { {kStyle,kModern}, {kGain,6}, {kCeiling,-1}, {kLookahead,4}, {kAttack,3}, {kRelease,150}, {kOversampling,2}, {kTruePeakLimit,1}, END };
static const PV loudModern[] = { {kStyle,kModern}, {kGain,10}, {kCeiling,-1}, {kLookahead,3}, {kAttack,2}, {kRelease,90}, {kOversampling,3}, {kTruePeakLimit,1}, END };
static const PV safeMaster[] = { {kStyle,kSafe}, {kGain,5}, {kCeiling,-1}, {kLookahead,6}, {kAttack,6}, {kRelease,200}, {kOversampling,2}, {kTruePeakLimit,1}, END };
static const PV master32[]   = { {kStyle,kClear}, {kGain,6}, {kCeiling,-1}, {kLookahead,5}, {kAttack,4}, {kRelease,140}, {kOversampling,5}, {kTruePeakLimit,1}, END };
static const PV aggEdm[]     = { {kStyle,kAggressive}, {kGain,12}, {kCeiling,-0.8f}, {kLookahead,1.5f}, {kAttack,1}, {kRelease,40}, {kOversampling,3}, {kTruePeakLimit,1}, END };
static const PV modernPop[]  = { {kStyle,kModern}, {kGain,8}, {kCeiling,-1}, {kLookahead,3}, {kAttack,2.5f}, {kRelease,110}, {kOversampling,2}, {kTruePeakLimit,1}, END };
static const PV trap808[]    = { {kStyle,kAggressive}, {kGain,9}, {kCeiling,-1}, {kLookahead,2}, {kAttack,1.5f}, {kRelease,60}, {kLinkTransient,1}, {kOversampling,3}, {kDcFilter,1}, END };
static const PV busGlue[]    = { {kStyle,kBus}, {kGain,4}, {kCeiling,-1}, {kLookahead,4}, {kAttack,4}, {kRelease,220}, {kOversampling,1}, END };
static const PV stemKey[]    = { {kStyle,kClear}, {kSidechain,1}, {kGain,6}, {kCeiling,-1}, {kLookahead,4}, {kAttack,3}, {kRelease,120}, {kOversampling,2}, END };
static const PV mixSafe[]    = { {kStyle,kSafe}, {kGain,2}, {kCeiling,-0.5f}, {kLookahead,3}, {kAttack,3}, {kRelease,150}, END };
static const PV dcTp[]       = { {kDcFilter,1}, {kTruePeakLimit,1}, {kCeiling,-1}, {kOversampling,2}, END };
static const PV podcast[]    = { {kStyle,kModern}, {kGain,6}, {kCeiling,-1.5f}, {kLookahead,5}, {kAttack,4}, {kRelease,180}, {kOversampling,2}, {kTruePeakLimit,1}, {kDcFilter,1}, END };
static const PV broadcastTp[] = { {kStyle,kSafe}, {kGain,4}, {kCeiling,-2}, {kLookahead,6}, {kAttack,6}, {kRelease,250}, {kOversampling,2}, {kTruePeakLimit,1}, {kDcFilter,1}, END };
} // namespace pv
#undef END

static const PresetDef kPresetDefs[] = {
    { "Init", "Init", pv::init },
    { "Transparent Master", "Mastering", pv::transparent }, { "Loud and Clean", "Mastering", pv::loudClean },
    { "Streaming -1 dBTP", "Mastering", pv::streaming }, { "CD 16-bit Final", "Mastering", pv::cd16 },
    { "Gentle Glue", "Mastering", pv::gentle }, { "Vinyl Premaster", "Mastering", pv::vinylPre },
    { "EDM", "Genres", pv::edm }, { "Hip-Hop", "Genres", pv::hiphop }, { "Rock", "Genres", pv::rock }, { "Pop", "Genres", pv::pop },
    { "Acoustic", "Genres", pv::acoustic }, { "Jazz", "Genres", pv::jazz }, { "Metal", "Genres", pv::metal }, { "Ambient", "Genres", pv::ambient },
    { "Drum Bus", "Mix & Bus", pv::drumBus }, { "Snare Peaks", "Mix & Bus", pv::snarePeak }, { "Vocal Catch", "Mix & Bus", pv::vocalCatch },
    { "Bass Leveler", "Mix & Bus", pv::bassLevel }, { "Wide Bus", "Mix & Bus", pv::wideBus }, { "Pumping", "Mix & Bus", pv::pumping },
    { "Safety Limiter", "Utility", pv::safety }, { "Broadcast", "Utility", pv::broadcast }, { "Soft Clipper", "Utility", pv::clipper },
    { "Dither to 16-bit", "Utility", pv::dither16 }, { "Dither to 24-bit", "Utility", pv::dither24 }, { "K-14 Metering", "Utility", pv::kMeter14 },
    // 2.0 (appended, so 1.x program numbers stay the same)
    { "Streaming True Peak", "Mastering", pv::streamTp }, { "Loud Modern Master", "Mastering", pv::loudModern },
    { "Safe Master", "Mastering", pv::safeMaster }, { "32x Clear Master", "Mastering", pv::master32 },
    { "Aggressive EDM", "Genres", pv::aggEdm }, { "Modern Pop", "Genres", pv::modernPop }, { "Trap 808", "Genres", pv::trap808 },
    { "Bus Glue", "Mix & Bus", pv::busGlue }, { "Stem (Sidechain)", "Mix & Bus", pv::stemKey }, { "Mix Bus Safety", "Mix & Bus", pv::mixSafe },
    { "DC + True Peak", "Utility", pv::dcTp }, { "Podcast Voice", "Utility", pv::podcast }, { "Broadcast True Peak", "Utility", pv::broadcastTp },
};
static constexpr uint32_t kNumPresets = sizeof(kPresetDefs) / sizeof(kPresetDefs[0]);

inline const Preset* presets() {
    static Preset table[kNumPresets];
    static bool done = false;
    if (done) return table;
    done = true;
    Engine e;
    for (uint32_t i = 0; i < kNumPresets; ++i) {
        table[i].name = kPresetDefs[i].name; table[i].category = kPresetDefs[i].category;
        std::copy(e.p, e.p + kParamCount, table[i].v);
        for (const PV* x = kPresetDefs[i].pv; x->id != kParamCount; ++x) table[i].v[x->id] = x->v;
    }
    return table;
}

} // namespace granite
