// Granite — custom vector UI (DPF/NanoVG). Original design. License: MIT.
#include "DistrhoUI.hpp"
#include "GranitePlugin.hpp"
#include "UserPresets.hpp"
#include "ResizeHandle.hpp"
#include "Help.hpp"
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
extern "C" __declspec(dllimport) void* __stdcall ShellExecuteA(void*, const char*, const char*, const char*, const char*, int);
#endif

START_NAMESPACE_DISTRHO

static void openURL(const char* url) {
#if defined(_WIN32)
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, 1);
#elif defined(__APPLE__)
    const std::string c = std::string("open '") + url + "' &"; if (std::system(c.c_str())) {}
#else
    const std::string c = std::string("xdg-open '") + url + "' >/dev/null 2>&1 &"; if (std::system(c.c_str())) {}
#endif
}
static bool gTipsOn = true;
static double nowSec() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

using namespace granite;
using DGL_NAMESPACE::ResizeHandle;

static constexpr float W = 1120.f, H = 730.f;

#define C_BG        Color(0x10, 0x10, 0x11)
#define C_PANEL     Color(0x19, 0x19, 0x1b)
#define C_PANEL_HI  Color(0x25, 0x25, 0x28)
#define C_LINE      Color(0x31, 0x31, 0x35)
#define C_TEXT      Color(0xe9, 0xe8, 0xe6)
#define C_DIM       Color(0x8d, 0x8c, 0x8a)
#define C_FAINT     Color(0x4f, 0x4e, 0x4d)
#define C_ACC       Color(0xd8, 0xcf, 0xc4)       // polished granite
#define C_IN        Color(0x4a, 0x4b, 0x50)       // input level
#define C_OUT       Color(0x9d, 0xc3, 0xd6)       // output level
#define C_GR        Color(0xe8, 0x50, 0x48)       // gain reduction
#define C_RMS       Color(0xf0, 0xc8, 0x6a)       // output RMS
#define C_DISPLAY   Color(0x0b, 0x0b, 0x0c)
#define C_LOUD      Color(0xb9, 0x9c, 0xe6)       // loudness
#define C_GOOD      Color(0x8f, 0xd1, 0x9a)       // on target
#define C_FAV       Color(0xff, 0xc8, 0x5a)       // favourite star

static Color withAlpha(const Color& c, float a) { Color r(c); r.alpha = a; return r; }

enum WType { Knob, Toggle, Segment };
struct Ctl { WType type; uint32_t param; float x, y, r; const char* label; int rows; };    // Segment: r = width, rows of buttons

static constexpr float LX = 16, LY = 64, LW = 850, LH = 320;          // level display
static constexpr float MX = 874, MY = 64, MW = 230, MH = 320;         // meters + loudness
static constexpr float PY = 394, PH = 320;
static constexpr float KX = 16, KW = 640, EX = 664, EW = 440;
static constexpr float SEG_H = 24;
static constexpr float STRIP_H = 92;                                  // compact layout: meter strip instead of the display
static constexpr float COMPACT_DY = -(LH - STRIP_H);                  // panels move up by this much
static constexpr float HC = H + COMPACT_DY;                           // compact height

static std::vector<Ctl> buildControls(float dy) {
    std::vector<Ctl> c;
    const float py = PY + dy;
    c.push_back({ Segment, kStyle, KX + 320, py + 30, 540, "", 2 });
    c.push_back({ Knob, kGain,      KX + 92,  py + 166, 46, "GAIN", 1 });
    c.push_back({ Knob, kLookahead, KX + 228, py + 166, 30, "LOOKAHEAD", 1 });
    c.push_back({ Knob, kAttack,    KX + 336, py + 166, 30, "ATTACK", 1 });
    c.push_back({ Knob, kRelease,   KX + 444, py + 166, 30, "RELEASE", 1 });
    c.push_back({ Knob, kCeiling,   KX + 562, py + 166, 40, "OUTPUT LEVEL", 1 });
    c.push_back({ Knob, kLinkTransient, EX + 54,  py + 80, 24, "TRANSIENT", 1 });
    c.push_back({ Knob, kLinkRelease,   EX + 136, py + 80, 24, "RELEASE", 1 });
    c.push_back({ Segment, kOversampling, EX + 316, py + 46, 228, "", 1 });
    c.push_back({ Toggle, kTruePeakLimit, EX + 222, py + 86, 0, "TRUE PEAK LIMITING", 1 });
    c.push_back({ Toggle, kTruePeak, EX + 222, py + 112, 0, "TRUE PEAK METERS", 1 });
    c.push_back({ Segment, kDither, EX + 104, py + 210, 176, "", 1 });
    c.push_back({ Segment, kShaping, EX + 316, py + 210, 228, "", 1 });
    c.push_back({ Segment, kMeterScale, EX + 104, py + 254, 176, "", 1 });
    c.push_back({ Segment, kDisplayRange, EX + 316, py + 254, 228, "", 1 });
    // 2.0
    c.push_back({ Segment, kSidechain, EX + 104, py + 166, 176, "", 1 });
    c.push_back({ Segment, kLoudTarget, EX + 316, py + 166, 228, "", 1 });
    c.push_back({ Segment, kDisplaySpeed, EX + 104, py + 298, 176, "", 1 });
    c.push_back({ Toggle, kUnityGain, KX + 56,  py + 290, 0, "UNITY GAIN", 1 });
    c.push_back({ Toggle, kAudition,  KX + 236, py + 290, 0, "AUDITION LIMITING", 1 });
    c.push_back({ Toggle, kDcFilter,  KX + 456, py + 290, 0, "DC FILTER", 1 });
    return c;
}

static const char* const* segLabels(uint32_t p, int& n) {
    static const char* const style[] = { "CLEAR", "PUNCHY", "DYNAMIC", "BALANCED", "AGGRESSIVE", "MODERN", "BUS", "SAFE" };
    static const char* const os[] = { "OFF", "2X", "4X", "8X", "16X", "32X" };
    static const char* const sc[] = { "INTERNAL", "EXTERNAL" };
    static const char* const tgt[] = { "OFF", "R128", "A/85", "-14", "-16" };
    static const char* const spd[] = { "NORMAL", "FAST", "SLOW", "OFF" };
    static const char* const dith[] = { "OFF", "16", "20", "24" };
    static const char* const shp[] = { "FLAT", "BASIC", "OPTIM.", "WEIGHT." };
    static const char* const scale[] = { "DBFS", "K-20", "K-14", "K-12" };
    static const char* const range[] = { "12 DB", "24 DB", "48 DB" };
    switch (p) {
    case kStyle: n = kNumStyles; return style;
    case kOversampling: n = kNumOversampling; return os;
    case kSidechain: n = 2; return sc;
    case kLoudTarget: n = kNumLoudTargets; return tgt;
    case kDisplaySpeed: n = 4; return spd;
    case kDither: n = 4; return dith;
    case kShaping: n = 4; return shp;
    case kMeterScale: n = 4; return scale;
    case kDisplayRange: n = 3; return range;
    default: n = 0; return nullptr;
    }
}

static constexpr float PS_X = 318.f, PS_Y = 20.f, PS_W = 220.f, PS_H = 28.f, PS_ARROW = 28.f;
static constexpr float A_X = 548.f, AB_W = 30.f, COPY_X = 612.f, COPY_W = 46.f;
static constexpr float UNDO_X = 668.f, REDO_X = 720.f, UR_W = 48.f, LEARN_X = 778.f, LEARN_W = 66.f, MIDI_X = 848.f, MIDI_W = 48.f, HELP_X = 902.f, HELP_W = 48.f;
static constexpr float CMP_X = 956.f, CMP_W = 72.f;
static constexpr float PM_COLW = 160.f, PM_ROW = 23.f, PM_HEAD = 22.f, PM_PAD = 8.f;
static constexpr int HIST = 1500;                 // 30 s at 50 points per second (the Slow display speed)
static int visiblePoints(float speed) { const int k = (int)clampf(std::round(speed), 0, 3); return k == 1 ? 250 : (k == 2 ? 1500 : 500); }

class GraniteUI : public UI {
public:
    GraniteUI() : UI((uint)W, (uint)H), resizeHandle(this) {
        loadSharedResources();
        Engine def;
        for (uint32_t i = 0; i < kParamCount; ++i) { val[i] = def.p[i]; defv[i] = def.p[i]; }
        val[kOutInL] = val[kOutInR] = val[kOutOutL] = val[kOutOutR] = -120;
        val[kOutLoudM] = val[kOutLoudS] = val[kOutLoudI] = val[kOutTruePeakMax] = -144; val[kOutLoudRange] = 0;
        for (int& m : paramCC) m = -1;
        const double sf = getScaleFactor();
        if (sf != 1.0) setSize((uint)(W * sf), (uint)(H * sf));
        setGeometryConstraints((uint)(W * 0.6 * sf), (uint)(HC * 0.6 * sf), false);
        if (isResizable()) resizeHandle.hide();
        userPresets = listUserPresets();
        std::copy(val, val + kParamCount, before);
        for (int i = 0; i < HIST; ++i) { hIn[i] = hOut[i] = hRms[i] = -120; hLoud[i] = -144; hGr[i] = 0; }
        resetPeaks();
    }

protected:
    float uiScale() const { return (float)getWidth() / W; }
    bool compact() const { return val[kCompact] > 0.5f; }
    float dy() const { return compact() ? COMPACT_DY : 0.f; }
    float py() const { return PY + dy(); }
    const std::vector<Ctl>& controls() const { static const std::vector<Ctl> n = buildControls(0), c = buildControls(COMPACT_DY); return compact() ? c : n; }
    // the window follows the layout: full height or compact, at the current width
    void fitHeight() { const float s = uiScale(); const uint h = (uint)std::lround((compact() ? HC : H) * s); if (getHeight() != h) setSize(getWidth(), h); }

    void setValue(uint32_t p, float v) {
        const ParamDef& d = paramDef(p);
        v = clampf(v, d.min, d.max);
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); if (p == kCompact) fitHeight(); repaint();
    }
    void commit(uint32_t p, float v) { editParameter(p, true); setValue(p, v); editParameter(p, false); }

    void parameterChanged(uint32_t i, float v) override {
        if (i >= kParamCount) return;
        if (i == kOutLearn) {
            if (v != lastLearnEcho && v > 0) {
                const int code = (int)v % 1000000 - 1, prm = code / 128, cc = code % 128;
                for (int& m : paramCC) if (m == cc) m = -1;
                if (prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
                learnWaiting = -1;
            }
            lastLearnEcho = v; repaint(); return;
        }
        if (paramDef(i).hints & kHintOutput) {        // meters: drawn on the idle tick
            val[i] = v;
            if (i == kOutInL || i == kOutInR) pkIn = std::max(pkIn, v);
            if (i == kOutOutL || i == kOutOutR) pkOut = std::max(pkOut, v);
            if (i == kOutGr) pkGr = std::max(pkGr, v);
            return;
        }
        if (val[i] != v) { val[i] = v; if (!gesture) before[i] = v; if (i == kCompact) fitHeight(); repaint(); }
    }
    void stateChanged(const char* key, const char* value) override {
        if (std::strcmp(key, "midimap") != 0) return;
        for (int& m : paramCC) m = -1;
        const char* q = value;
        while (q && *q) {
            int cc = -1, prm = -1;
            if (std::sscanf(q, "%d:%d", &cc, &prm) == 2 && prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
            const char* s = std::strchr(q, ';'); if (!s) break; q = s + 1;
        }
        repaint();
    }
    void resetPeaks() { pkIn = pkOut = -120; pkGr = 0; }

    // ---- undo / redo / A-B (last snapshot entry = A/B slot) ----------------------------
    typedef std::vector<float> Snap;
    Snap snapshot() const { Snap s(val, val + kParamCount); s.push_back((float)abSlot); return s; }
    bool differs(const float* a, const float* b) const {
        for (uint32_t i = 0; i < kParamCount; ++i) if (isSetting(i) && a[i] != b[i]) return true;
        return false;
    }
    void beginGesture() { gesture = true; std::copy(val, val + kParamCount, before); }
    void endGesture() {
        gesture = false;
        if (differs(before, val)) {
            Snap u(before, before + kParamCount); u.push_back((float)abSlot);
            undoStack.push_back(u); if (undoStack.size() > 200) undoStack.erase(undoStack.begin());
            redoStack.clear();
        }
        std::copy(val, val + kParamCount, before); repaint();
    }
    void applyAll(const float* s) { for (uint32_t i = 0; i < kParamCount; ++i) if (isSetting(i) && val[i] != s[i]) commit(i, s[i]); }
    void restoreSnap(const Snap& s) {
        if (s.size() > kParamCount && (int)s[kParamCount] != abSlot) { abStore[abSlot] = snapshot(); abSlot = (int)s[kParamCount]; }
        applyAll(s.data()); std::copy(val, val + kParamCount, before);
    }
    void undo() { if (undoStack.empty()) return; redoStack.push_back(snapshot()); restoreSnap(undoStack.back()); undoStack.pop_back(); flash("Undo"); }
    void redo() { if (redoStack.empty()) return; undoStack.push_back(snapshot()); restoreSnap(redoStack.back()); redoStack.pop_back(); flash("Redo"); }
    void switchAB(int to) {
        if (to == abSlot) return;
        abStore[abSlot] = snapshot(); if (abStore[to].empty()) abStore[to] = snapshot();
        beginGesture(); applyAll(abStore[to].data()); endGesture();
        abSlot = to; flash(to == 0 ? "Setting A" : "Setting B");
    }
    void copyAB() { abStore[1 - abSlot] = snapshot(); flash(abSlot == 0 ? "Copied A to B" : "Copied B to A"); }

    // ---- formatting ---------------------------------------------------------
    void fmt(uint32_t p, char* b, size_t n) const {
        const float v = val[p];
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) { snprintf(b, n, "%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))]); return; }
        const ParamDef& d = paramDef(p);
        if (d.hints & kHintBoolean) { snprintf(b, n, "%s", v > 0.5f ? "On" : "Off"); return; }
        switch (p) {
        case kGain: snprintf(b, n, "%+.1f dB", v); return;
        case kCeiling: snprintf(b, n, "%.1f dB", v); return;
        case kLookahead: case kAttack: snprintf(b, n, v < 1 ? "%.2f ms" : "%.1f ms", v); return;
        case kRelease: if (v >= 1000) snprintf(b, n, "%.2f s", v / 1000); else snprintf(b, n, v < 10 ? "%.1f ms" : "%.0f ms", v); return;
        default: snprintf(b, n, "%.0f%%", v * 100);
        }
    }
    bool parse(uint32_t p, const std::string& in, float& out) const {
        std::string s; for (char ch : in) if (!std::isspace((unsigned char)ch)) s += (char)std::tolower((unsigned char)ch);
        if (s.empty()) return false;
        char* end = nullptr; double x = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) return false;
        std::string suf(end);
        if (p == kRelease && suf == "s") x *= 1000;
        if (p == kLinkTransient || p == kLinkRelease) { out = (float)(x / 100.0); return true; }
        out = (float)x; return true;
    }

    // ---- drawing ------------------------------------------------------------
    void onNanoDisplay() override {
        const float s = uiScale();
        save(); scale(s, s);
        beginPath(); rect(0, 0, W, H); fillColor(C_BG); fill();
        fontFace(NANOVG_DEJAVU_SANS_TTF);

        // header: a granite block, speckled
        beginPath(); roundedRect(16, 18, 30, 30, 3); fillColor(Color(0x6a, 0x66, 0x62)); fill();
        static const float sp[][3] = { {21,23,1.6f},{30,21,1.2f},{39,26,1.8f},{24,33,1.4f},{34,31,2.f},{41,40,1.3f},{20,42,1.7f},{29,43,1.1f},{36,37,1.f},{26,27,0.9f} };
        for (int i = 0; i < 10; ++i) { beginPath(); circle(sp[i][0], sp[i][1], sp[i][2]); fillColor(i % 3 == 0 ? Color(0x1c, 0x1b, 0x1a) : (i % 3 == 1 ? Color(0xe8, 0xe2, 0xda) : Color(0xb4, 0x8a, 0x78))); fill(); }
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(56, 34, "GRANITE", nullptr);
        fontSize(9.5f); textLetterSpacing(2.5f); fillColor(C_DIM); text(222, 29, "LIMITER", nullptr);
        fontSize(7); textLetterSpacing(1.1f); fillColor(C_FAINT); text(222, 41, "EPSILON AUDIO 2.1", nullptr);
        drawPresetBar();
        for (int k = 0; k < 2; ++k) {
            const float x = A_X + k * AB_W; const bool on = abSlot == k, hot = hoverHeader == 6 + k;
            beginPath(); roundedRect(x, PS_Y, AB_W - 2, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.25f) : C_PANEL); fill();
            strokeColor(on ? C_ACC : (hot ? C_DIM : C_LINE)); strokeWidth(1); stroke();
            fontSize(10); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM);
            text(x + (AB_W - 2) / 2, PS_Y + PS_H / 2 + 0.5f, k == 0 ? "A" : "B", nullptr);
        }
        drawHeaderButton(COPY_X, COPY_W, "COPY", false, hoverHeader == 8, false);
        drawHeaderButton(UNDO_X, UR_W, "UNDO", false, hoverHeader == 3, undoStack.empty());
        drawHeaderButton(REDO_X, UR_W, "REDO", false, hoverHeader == 4, redoStack.empty());
        drawHeaderButton(LEARN_X, LEARN_W, learnMode ? "LEARNING" : "LEARN", learnMode, hoverHeader == 1, false);
        drawHeaderButton(MIDI_X, MIDI_W, "MIDI", val[kMidiOn] > 0.5f, hoverHeader == 2, false);
        drawHeaderButton(HELP_X, HELP_W, "HELP", helpOpen, hoverHeader == 5, false);
        drawHeaderButton(CMP_X, CMP_W, compact() ? "FULL" : "COMPACT", false, hoverHeader == 9, false);
        { char b[24]; snprintf(b, sizeof b, "%.1f ms", latencyMs);
          fontSize(7); textLetterSpacing(1.4f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(C_FAINT); text(W - 16, 28, "LATENCY", nullptr);
          fontSize(9.5f); textLetterSpacing(0.4f); fillColor(C_DIM); text(W - 16, 41, b, nullptr); }

        if (compact()) drawStrip(); else { drawLevelDisplay(); drawMeters(); }
        const float Y = py();
        panel(KX, Y, KW, PH); panel(EX, Y, EW, PH);
        drawCaption(EX + 95, Y + 22, "CHANNEL LINK"); drawCaption(EX + 316, Y + 22, "OVERSAMPLING");
        drawCaption(EX + 104, Y + 148, "SIDECHAIN"); drawCaption(EX + 316, Y + 148, "LOUDNESS TARGET");
        drawCaption(EX + 104, Y + 192, "DITHER"); drawCaption(EX + 316, Y + 192, "NOISE SHAPING");
        drawCaption(EX + 104, Y + 236, "METER SCALE"); drawCaption(EX + 316, Y + 236, "DISPLAY RANGE");
        drawCaption(EX + 104, Y + 280, "DISPLAY SPEED");
        beginPath(); moveTo(EX + 16, Y + 132); lineTo(EX + EW - 16, Y + 132); moveTo(KX + 16, Y + 252); lineTo(KX + KW - 16, Y + 252); strokeColor(C_LINE); strokeWidth(1); stroke();

        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) drawCtl(cs[i], i == active || i == hover);
        if (editing >= 0) drawEditBox();
        drawStatus();
        if (menuOpen) drawPresetMenu();
        if (saving) drawSaveBox();
        if (helpOpen) drawHelp();
        else if (tipShown && gTipsOn) drawTooltip();
        restore();
    }
    void panel(float x, float y, float w, float h) { beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke(); }
    void displayBox(float x, float y, float w, float h) { beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); strokeWidth(1); stroke(); }
    void drawCaption(float cx, float y, const char* t) { fontSize(7.5f); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT); text(cx, y, t, nullptr); }
    void drawHeaderButton(float x, float w, const char* label, bool on, bool hot, bool disabled) {
        beginPath(); roundedRect(x, PS_Y, w, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.2f) : C_PANEL); fill();
        strokeColor(on ? C_ACC : (hot && !disabled ? C_DIM : C_LINE)); strokeWidth(1); stroke();
        fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(disabled ? C_FAINT : (on ? C_TEXT : C_DIM));
        text(x + w / 2, PS_Y + PS_H / 2 + 0.5f, label, nullptr);
    }

    // level display: top = a little headroom above 0 dBFS, bottom = -range
    float rangeDb() const { return displayRangeDb(val[kDisplayRange]); }
    float headDb() const { return rangeDb() / 4.f; }
    float yOfDb(float db, float top, float h) const { return top + h * clampf((headDb() - db) / (headDb() + rangeDb()), 0.f, 1.f); }
    float dbOfY(float y, float top, float h) const { return headDb() - (y - top) / h * (headDb() + rangeDb()); }
    float dispTop() const { return LY + 10; }
    float dispH() const { return LH - 30; }
    // the visible history: the last `n` points, pooled into at most `cols` columns (max for levels and GR,
    // last value for loudness), oldest first
    int pooled(std::vector<float>& in, std::vector<float>& out, std::vector<float>& gr, std::vector<float>& rms, std::vector<float>& lu) const {
        const int n = visiblePoints(val[kDisplaySpeed]), cols = 500, step = (n + cols - 1) / cols, m = n / step;
        in.assign(m, -120); out.assign(m, -120); gr.assign(m, 0); rms.assign(m, -120); lu.assign(m, -144);
        for (int k = 0; k < m; ++k) for (int j = 0; j < step; ++j) {
            const int idx = ((hPos - n + k * step + j) % HIST + HIST) % HIST;
            in[k] = std::max(in[k], hIn[idx]); out[k] = std::max(out[k], hOut[idx]); gr[k] = std::max(gr[k], hGr[idx]);
            rms[k] = std::max(rms[k], hRms[idx]); lu[k] = hLoud[idx];
        }
        return m;
    }
    float targetLufs() const { return loudTargetLufs((int)std::round(val[kLoudTarget])); }
    bool targetOn() const { return val[kLoudTarget] > 0.5f; }
    void drawLevelDisplay() {
        displayBox(LX, LY, LW, LH);
        const float top = dispTop(), h = dispH(), R = rangeDb(), ko = kOffsetDb(val[kMeterScale]);
        const float step = R <= 12 ? 3.f : (R <= 24 ? 6.f : 12.f);
        fontSize(8); textLetterSpacing(1); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT);
        beginPath();
        for (float db = 0; db >= -R; db -= step) { const float y = yOfDb(db, top, h); moveTo(LX + 6, y); lineTo(LX + LW - 6, y); }
        strokeColor(Color(255, 255, 255, 8 / 255.f)); strokeWidth(1); stroke();
        for (float db = 0; db >= -R + 0.1f; db -= step) { char b[12]; snprintf(b, sizeof b, "%+.0f", db + ko); text(LX + 8, yOfDb(db, top, h) - 6, (db == 0 && ko == 0) ? "0" : b, nullptr); }
        const float x0 = LX + 34, x1 = LX + LW - 8;
        const bool off = std::round(val[kDisplaySpeed]) == 3;
        std::vector<float> in, out, gr, rms, lu; const int m = off ? 0 : pooled(in, out, gr, rms, lu);
        const float dx = m > 1 ? (x1 - x0) / (m - 1) : 0;
        const float bot = top + h;
        scissor(LX + 1, LY + 1, LW - 2, LH - 2);
        if (m > 1) {
            // input (grey), with the part the limiter removed in red, then the output on top
            beginPath(); moveTo(x0, bot);
            for (int k = 0; k < m; ++k) lineTo(x0 + k * dx, yOfDb(in[k], top, h));
            lineTo(x1, bot); closePath(); fillColor(C_IN); fill();
            beginPath();
            for (int k = 0; k < m; ++k) { const float x = x0 + k * dx, y = yOfDb(in[k], top, h); if (!k) moveTo(x, y); else lineTo(x, y); }
            for (int k = m - 1; k >= 0; --k) lineTo(x0 + k * dx, yOfDb(std::min(out[k], in[k]), top, h));
            closePath(); fillColor(withAlpha(C_GR, 0.8f)); fill();
            beginPath(); moveTo(x0, bot);
            for (int k = 0; k < m; ++k) lineTo(x0 + k * dx, yOfDb(std::min(out[k], in[k]), top, h));
            lineTo(x1, bot); closePath(); fillPaint(linearGradient(0, top, 0, bot, Color(0x5a, 0x6e, 0x7a), Color(0x2e, 0x36, 0x3c))); fill();
            beginPath();
            for (int k = 0; k < m; ++k) { const float x = x0 + k * dx, y = yOfDb(out[k], top, h); if (!k) moveTo(x, y); else lineTo(x, y); }
            strokeColor(C_OUT); strokeWidth(1.2f); stroke();
            // gain reduction trace from the top (same dB scale)
            auto grY = [&](float g) { return top + h * clampf(g / (headDb() + R), 0.f, 1.f); };
            beginPath();
            for (int k = 0; k < m; ++k) { const float x = x0 + k * dx, y = grY(gr[k]); if (!k) moveTo(x, y); else lineTo(x, y); }
            strokeColor(withAlpha(C_GR, 0.9f)); strokeWidth(1.4f); stroke();
            // output RMS, or momentary loudness when a loudness target is set (LUFS on the same dB grid)
            beginPath();
            for (int k = 0; k < m; ++k) { const float x = x0 + k * dx, y = yOfDb(targetOn() ? lu[k] : rms[k], top, h); if (!k) moveTo(x, y); else lineTo(x, y); }
            strokeColor(withAlpha(targetOn() ? C_LOUD : C_RMS, 0.85f)); strokeWidth(1.2f); stroke();
            // peak gain reduction labels: the deepest point in each stretch of the display
            const int span = std::max(8, (int)(70 / std::max(dx, 0.01f)));
            fontSize(8.5f); textLetterSpacing(0.3f); textAlign(ALIGN_CENTER | ALIGN_TOP);
            float lastX = -1000;
            for (int k0 = 0; k0 < m; k0 += span) {
                int best = -1; float g = 0.5f;
                for (int k = k0; k < std::min(m, k0 + span); ++k) if (gr[k] > g) { g = gr[k]; best = k; }
                if (best < 2 || best > m - 3 || x0 + best * dx - lastX < 56) continue;
                lastX = x0 + best * dx;
                char b[12]; snprintf(b, sizeof b, "-%.1f", g); const float x = x0 + best * dx, y = grY(g) + 3;
                fillColor(Color(0x0b, 0x0b, 0x0c, 0.7f)); beginPath(); roundedRect(x - 15, y, 30, 12, 2); fill();
                fillColor(C_GR); text(x, y + 1, b, nullptr);
            }
        } else if (off) {
            fontSize(9); textLetterSpacing(2); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT); text(LX + LW / 2, top + h / 2, "DISPLAY OFF", nullptr);
        }
        if (targetOn()) {           // the target, as a band of +-1 LU
            const float t = targetLufs(), ya = yOfDb(t + 1, top, h), yb = yOfDb(t - 1, top, h);
            beginPath(); rect(x0, ya, x1 - x0, yb - ya); fillColor(withAlpha(C_LOUD, 0.08f)); fill();
            beginPath(); moveTo(x0, yOfDb(t, top, h)); lineTo(x1, yOfDb(t, top, h)); strokeColor(withAlpha(C_LOUD, 0.5f)); strokeWidth(1); stroke();
            char b[32]; snprintf(b, sizeof b, "TARGET %.0f LUFS", t); fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_LEFT | ALIGN_BOTTOM); fillColor(C_LOUD); text(x0 + 6, yOfDb(t, top, h) - 3, b, nullptr);
        }
        resetScissor();
        // output level line (dashed, draggable)
        const float cy = yOfDb(val[kCeiling], top, h); const bool hot = hoverCeil || dragCeil;
        for (float x = x0; x < x1; x += 10) { beginPath(); moveTo(x, cy); lineTo(std::min(x + 5, x1), cy); strokeColor(withAlpha(C_ACC, hot ? 1.f : 0.75f)); strokeWidth(hot ? 2.f : 1.2f); stroke(); }
        char b[40]; snprintf(b, sizeof b, val[kTruePeakLimit] > 0.5f ? "OUTPUT %.1f dBTP" : "OUTPUT %.1f dB", val[kCeiling]);
        fontSize(9); textLetterSpacing(0.8f); textAlign(ALIGN_RIGHT | ALIGN_BOTTOM); fillColor(C_ACC); text(x1 - 2, cy - 3, b, nullptr);
        fontSize(8); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_BOTTOM);
        fillColor(C_DIM); text(LX + 34, LY + LH - 4, "INPUT", nullptr); fillColor(C_OUT); text(LX + 86, LY + LH - 4, "OUTPUT", nullptr);
        fillColor(C_GR); text(LX + 146, LY + LH - 4, "GAIN REDUCTION", nullptr);
        if (targetOn()) { fillColor(C_LOUD); text(LX + 262, LY + LH - 4, "LOUDNESS (M)", nullptr); } else { fillColor(C_RMS); text(LX + 262, LY + LH - 4, "RMS", nullptr); }
        textAlign(ALIGN_RIGHT | ALIGN_BOTTOM); fillColor(C_FAINT);
        static const char* const spanTxt[] = { "10 S", "5 S", "30 S", "" };
        text(x1, LY + LH - 4, spanTxt[(int)clampf(std::round(val[kDisplaySpeed]), 0, 3)], nullptr);
        drawMonitorBadge(LX + LW / 2, LY + 18);
    }
    // a reminder while the output is not the real result
    void drawMonitorBadge(float cx, float cy) {
        const bool a = val[kAudition] > 0.5f, u = val[kUnityGain] > 0.5f, ext = val[kSidechain] > 0.5f;
        if (!a && !u && !ext) return;
        std::string t = a && u ? "AUDITION LIMITING · UNITY GAIN" : (a ? "AUDITION LIMITING" : (u ? "UNITY GAIN" : ""));
        if (ext) t = t.empty() ? "EXTERNAL SIDECHAIN" : t + " · EXTERNAL SIDECHAIN";
        fontSize(8.5f); textLetterSpacing(1.6f); Rectangle<float> bb; textBounds(0, 0, t.c_str(), nullptr, bb); const float w = bb.getWidth() + 20;
        beginPath(); roundedRect(cx - w / 2, cy - 9, w, 18, 9); fillColor(withAlpha((a || u) ? C_GR : C_ACC, 0.18f)); fill(); strokeColor((a || u) ? C_GR : C_ACC); strokeWidth(1); stroke();
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor((a || u) ? C_GR : C_ACC); text(cx, cy + 0.5f, t.c_str(), nullptr);
    }
    // loudness readouts: M, S, I, LRA, TP max; I against the target
    void drawLoudness(float x, float y, float w, bool row) {
        const float vM = val[kOutLoudM], vS = val[kOutLoudS], vI = val[kOutLoudI], vR = val[kOutLoudRange], vT = val[kOutTruePeakMax];
        const char* names[5] = { "M", "S", "I", "LRA", "TP" };
        const char* units[5] = { "LUFS", "LUFS", "LUFS", "LU", "dBTP" };
        const float vals[5] = { vM, vS, vI, vR, vT };
        for (int k = 0; k < 5; ++k) {
            char b[16];
            if (k == 3) snprintf(b, sizeof b, "%.1f", vals[k]); else if (vals[k] < -100) snprintf(b, sizeof b, "-inf"); else snprintf(b, sizeof b, "%.1f", vals[k]);
            Color c = C_TEXT;
            if (k == 2 && targetOn() && vI > -100) c = std::fabs(vI - targetLufs()) <= 1.f ? C_GOOD : C_RMS;
            if (k == 4 && vT > val[kCeiling] + 0.05f) c = C_GR;
            const float xx = row ? x + k * (w / 5) : x, yy = row ? y : y + k * 40;
            fontSize(7.5f); textLetterSpacing(1.2f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_FAINT); text(xx, yy, names[k], nullptr);
            fontSize(row ? 13.f : 15.f); textLetterSpacing(0.3f); fillColor(c); text(xx, yy + 10, b, nullptr);
            fontSize(7); textLetterSpacing(0.8f); fillColor(C_FAINT); text(xx + (row ? 44 : 50), yy + (row ? 15 : 17), units[k], nullptr);
        }
        if (!row) {
            fontSize(7); textLetterSpacing(1.2f); textAlign(ALIGN_LEFT | ALIGN_BOTTOM); fillColor(C_FAINT);
            if (targetOn()) { char b[32]; snprintf(b, sizeof b, "TARGET %.0f LUFS", targetLufs()); fillColor(C_LOUD); text(x, MY + MH - 22, b, nullptr); fillColor(C_FAINT); }
            text(x, MY + MH - 10, "CLICK TO RESET", nullptr);
        }
    }
    void drawMeters() {
        displayBox(MX, MY, MW, MH);
        drawBarMeters(MX, MY, MH);
        beginPath(); moveTo(MX + 120, MY + 10); lineTo(MX + 120, MY + MH - 10); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(7.5f); textLetterSpacing(1.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(MX + 132, MY + 12, "LOUDNESS", nullptr);
        drawLoudness(MX + 132, MY + 36, 90, false);
    }
    void drawBarMeters(float mx, float my, float mh) {
        const float top = my + 40, bot = my + mh - 22, h = bot - top, R = rangeDb(), ko = kOffsetDb(val[kMeterScale]);
        auto y = [&](float db) { return yOfDb(db, top, h); };
        auto bar = [&](float x, float w, float db, const Color& c) {
            beginPath(); rect(x, top, w, h); fillColor(C_PANEL_HI); fill();
            if (db > -R - 1) { const float yy = y(db); beginPath(); rect(x, yy, w, bot - yy); fillColor(c); fill();
                if (db > 0.01f) { beginPath(); rect(x, top, w, 3); fillColor(C_GR); fill(); } }
        };
        bar(mx + 10, 8, val[kOutInL], Color(0x7a, 0x7b, 0x80)); bar(mx + 20, 8, val[kOutInR], Color(0x7a, 0x7b, 0x80));
        { const float x = mx + 46; beginPath(); rect(x, top, 10, h); fillColor(C_PANEL_HI); fill();
          const float g = clampf(val[kOutGr] / (headDb() + R), 0.f, 1.f); if (g > 0.002f) { beginPath(); rect(x, top, 10, h * g); fillColor(C_GR); fill(); } }
        bar(mx + 82, 8, val[kOutOutL], C_OUT); bar(mx + 92, 8, val[kOutOutR], C_OUT);
        // ceiling tick on the output meters, 0 dB tick on input
        beginPath(); moveTo(mx + 80, y(val[kCeiling])); lineTo(mx + 102, y(val[kCeiling])); strokeColor(C_ACC); strokeWidth(1); stroke();
        beginPath(); moveTo(mx + 8, y(0)); lineTo(mx + 30, y(0)); strokeColor(C_FAINT); stroke();
        // scale
        fontSize(7); textLetterSpacing(0.4f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT);
        const float step = R <= 12 ? 3.f : (R <= 24 ? 6.f : 12.f);
        for (float db = 0; db >= -R + 0.1f; db -= step) { char b[12]; snprintf(b, sizeof b, "%.0f", db + ko); text(mx + 69, y(db), b, nullptr); }
        fontSize(7.5f); textLetterSpacing(1.2f); fillColor(C_FAINT);
        text(mx + 19, my + 12, "IN", nullptr); text(mx + 51, my + 12, "GR", nullptr); text(mx + 91, my + 12, val[kTruePeak] > 0.5f ? "OUT TP" : "OUT", nullptr);
        char b[16]; fontSize(8.5f); textLetterSpacing(0.2f);
        snprintf(b, sizeof b, "%.1f", pkIn + ko); fillColor(pkIn > 0 ? C_GR : C_DIM); text(mx + 19, my + 27, pkIn > -100 ? b : "-inf", nullptr);
        snprintf(b, sizeof b, "-%.1f", pkGr); fillColor(C_GR); text(mx + 51, my + 27, b, nullptr);
        snprintf(b, sizeof b, "%.1f", pkOut + ko); fillColor(pkOut > val[kCeiling] + 0.05f ? C_GR : C_OUT); text(mx + 91, my + 27, pkOut > -100 ? b : "-inf", nullptr);
        fontSize(7); fillColor(C_FAINT);
        if (ko > 0) { char kb[16]; snprintf(kb, sizeof kb, "K-%.0f", ko); text(mx + 60, my + mh - 10, kb, nullptr); }
        else text(mx + 60, my + mh - 10, "dBFS", nullptr);
    }
    // compact layout: horizontal meters and the loudness readouts in one strip
    void drawStrip() {
        displayBox(LX, LY, W - 2 * LX, STRIP_H - 8);
        const float x0 = LX + 46, x1 = LX + 470, R = rangeDb();
        auto xOf = [&](float db) { return x0 + (x1 - x0) * clampf((db + R) / (R + headDb()), 0.f, 1.f); };
        auto hbar = [&](float y, float db, const Color& c, const char* name) {
            fontSize(7.5f); textLetterSpacing(1.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(LX + 12, y + 4, name, nullptr);
            beginPath(); rect(x0, y, x1 - x0, 8); fillColor(C_PANEL_HI); fill();
            if (db > -R) { beginPath(); rect(x0, y, xOf(db) - x0, 8); fillColor(c); fill(); }
        };
        hbar(LY + 12, std::max(val[kOutInL], val[kOutInR]), Color(0x7a, 0x7b, 0x80), "IN");
        { const float y = LY + 34; fontSize(7.5f); textLetterSpacing(1.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(LX + 12, y + 4, "GR", nullptr);
          beginPath(); rect(x0, y, x1 - x0, 8); fillColor(C_PANEL_HI); fill();
          const float g = clampf(val[kOutGr] / (headDb() + R), 0.f, 1.f); if (g > 0.002f) { beginPath(); rect(x1 - (x1 - x0) * g, y, (x1 - x0) * g, 8); fillColor(C_GR); fill(); } }
        hbar(LY + 56, std::max(val[kOutOutL], val[kOutOutR]), C_OUT, "OUT");
        beginPath(); moveTo(xOf(val[kCeiling]), LY + 52); lineTo(xOf(val[kCeiling]), LY + 68); strokeColor(C_ACC); strokeWidth(1.2f); stroke();
        char b[24]; fontSize(9); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        snprintf(b, sizeof b, "GR -%.1f dB", pkGr); fillColor(C_GR); text(x1 + 14, LY + 38, b, nullptr);
        snprintf(b, sizeof b, "OUT %.1f", pkOut + kOffsetDb(val[kMeterScale])); fillColor(pkOut > val[kCeiling] + 0.05f ? C_GR : C_OUT); text(x1 + 14, LY + 60, pkOut > -100 ? b : "OUT -inf", nullptr);
        drawLoudness(LX + 620, LY + 22, 470, true);
        drawMonitorBadge(LX + 690, LY + 10);
    }
    void drawKnobBody(float x, float y, float r, float n, bool hot, Color col) {
        const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f, ang = a0 + (a1 - a0) * n;
        const float tw = r > 30 ? 6.f : (r > 18 ? 4.f : 3.f);
        beginPath(); arc(x, y, r, a0, a1, NanoVG::CW); strokeColor(C_LINE); strokeWidth(tw); lineCap(ROUND); stroke();
        if (ang - a0 > 0.001f) { beginPath(); arc(x, y, r, a0, ang, NanoVG::CW); strokeColor(col); stroke(); }
        const float cr = r - (r > 30 ? 12 : (r > 18 ? 8 : 6));
        beginPath(); circle(x, y, cr);
        fillPaint(radialGradient(x - cr * 0.3f, y - cr * 0.4f, 0, cr * 1.4f, hot ? Color(0x3d, 0x3d, 0x41) : C_PANEL_HI, Color(0x14, 0x14, 0x15)));
        fill(); strokeColor(Color(255, 255, 255, 18 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(x + std::cos(ang) * cr * 0.35f, y + std::sin(ang) * cr * 0.35f); lineTo(x + std::cos(ang) * cr * 0.9f, y + std::sin(ang) * cr * 0.9f);
        strokeColor(C_TEXT); strokeWidth(r > 30 ? 3.f : 2.f); lineCap(ROUND); stroke();
    }
    void drawCtl(const Ctl& c, bool hot) {
        const uint32_t p = c.param; char buf[40];
        switch (c.type) {
        case Knob: {
            drawKnobBody(c.x, c.y, c.r, toNorm(p, val[p]), hot, p == kGain ? C_GR : C_ACC);
            const bool big = c.r >= 30;
            fontSize(big ? 9.f : 8.f); textLetterSpacing(big ? 1.8f : 1.4f); textAlign(ALIGN_CENTER | ALIGN_TOP);
            fillColor(C_DIM); text(c.x, c.y + c.r + 6, c.label, nullptr);
            fmt(p, buf, sizeof buf); fontSize(big ? 11.5f : 10.5f); textLetterSpacing(0.5f); fillColor(hot ? C_ACC : (big ? C_TEXT : Color(0xc8, 0xc6, 0xc2)));
            text(c.x, c.y + c.r + (big ? 19 : 18), buf, nullptr);
            if (p == kAttack && val[kAttack] > val[kLookahead] + 0.01f) {        // attack is limited to the look-ahead
                fontSize(7.5f); textLetterSpacing(1); fillColor(C_FAINT); text(c.x, c.y - c.r - 16, "LIMITED BY LOOKAHEAD", nullptr);
            }
            break; }
        case Toggle: {
            const bool on = val[p] > 0.5f;
            const float w = 34, h = 18, x = c.x - w / 2, y = c.y - h / 2;
            beginPath(); roundedRect(x, y, w, h, h / 2); fillColor(on ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(on ? C_ACC : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(on ? x + w - h / 2 : x + h / 2, c.y, 6); fillColor(on ? C_ACC : C_DIM); fill();
            fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(c.x + w / 2 + 8, c.y + 0.5f, c.label, nullptr);
            break; }
        case Segment: {
            int n = 0; const char* const* names = segLabels(p, n);
            const int cur = (int)(val[p] + .5f), per = (n + c.rows - 1) / c.rows; const float sw = c.r / per, x0 = c.x - c.r / 2;
            const bool dim = p == kShaping && val[kDither] < 0.5f;
            for (int i = 0; i < n; ++i) {
                const float x = x0 + sw * (i % per), yy = c.y + (i / per) * (SEG_H + 4);
                beginPath(); roundedRect(x + 1.5f, yy - SEG_H / 2, sw - 3, SEG_H, 5); fillColor(i == cur ? (dim ? C_FAINT : C_ACC) : C_PANEL_HI); fill();
                fontSize(p == kStyle ? 9.5f : 8.5f); textLetterSpacing(n > 5 && p != kStyle ? 0.8f : 1.4f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(i == cur ? C_BG : (dim ? C_FAINT : C_DIM)); text(x + sw / 2, yy + 0.5f, names[i], nullptr);
            }
            break; }
        }
        if (learnMode) {
            const int cc = paramCC[p]; const bool waiting = learnWaiting == (int)p;
            if (cc >= 0 || waiting) {
                const float by = c.type == Knob ? c.y - c.r - 12 : c.y - SEG_H / 2 - 8;
                snprintf(buf, sizeof buf, waiting ? "?" : "CC%d", cc);
                fontSize(8.5f); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                const float bw = waiting ? 16 : 34;
                beginPath(); roundedRect(c.x - bw / 2, by - 7, bw, 14, 3); fillColor(waiting ? C_ACC : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fillColor(waiting ? C_BG : C_ACC); text(c.x, by + 0.5f, buf, nullptr);
            }
        }
    }
    void drawStatus() {
        std::string msg;
        if (statusFlash > 0) msg = statusText;
        else if (learnMode) msg = learnWaiting >= 0 ? std::string("Move a MIDI controller to assign: ") + paramDef(learnWaiting).name : "MIDI Learn: click a control, then move a controller.  Right-click a control to unassign.";
        else if (dragGain) { char b[48]; snprintf(b, sizeof b, "Gain %+.1f dB", val[kGain]); msg = b; }
        if (!msg.empty()) { fontSize(10); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_ACC); text(LX + 40, LY + 18, msg.c_str(), nullptr); }
        if (learnMode && learnWaiting < 0) { textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(hoverClear ? C_TEXT : C_DIM); text(LX + LW - 12, LY + 18, "CLEAR ALL ASSIGNMENTS", nullptr); }
        if (statusFlash > 0) { --statusFlash; repaint(); }
    }
    void flash(const std::string& s) { statusText = s; statusFlash = 60; repaint(); }
    void drawEditBox() {
        const Ctl& c = controls()[editing];
        const float w = 90, h = 22, x = c.x - w / 2, y = c.y + c.r + 16;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(12); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(c.x, y + h / 2 + 0.5f, (editBuf + "|").c_str(), nullptr);
    }

    // ---- presets ------------------------------------------------------------------
    bool matchesValues(const float* v) const {
        for (uint32_t i = 0; i < kParamCount; ++i) {
            if (!isSetting(i)) continue;
            if (std::fabs(val[i] - v[i]) > 1e-4f * std::max(1.f, std::fabs(v[i]))) return false;
        }
        return true;
    }
    void applyValues(const float* v) { beginGesture(); applyAll(v); endGesture(); }
    void applyFactory(int idx) { idx = (idx % (int)kNumPresets + (int)kNumPresets) % (int)kNumPresets; applyValues(presets()[idx].v); curFactory = idx; curUser.clear(); repaint(); }
    void applyUser(const std::string& name) { float v[kParamCount]; if (!loadUserPreset(name, v)) return; applyValues(v); std::copy(v, v + kParamCount, userVals); curUser = name; curFactory = -1; repaint(); }
    void programLoaded(uint32_t index) override { curFactory = (int)index; curUser.clear(); repaint(); }
    void step(int d) {
        const int nf = (int)kNumPresets, nu = (int)userPresets.size(), total = nf + nu;
        int cur = curFactory; if (!curUser.empty()) for (int i = 0; i < nu; ++i) if (userPresets[i] == curUser) cur = nf + i;
        if (cur < 0) cur = 0;
        const int next = ((cur + d) % total + total) % total;
        if (next < nf) applyFactory(next); else applyUser(userPresets[next - nf]);
    }
    void drawPresetBar() {
        const float x = PS_X, y = PS_Y, w = PS_W, h = PS_H, a = PS_ARROW;
        beginPath(); roundedRect(x, y, w, h, 5); fillColor(C_PANEL); fill(); strokeColor(menuOpen ? C_ACC : C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(x + a, y + 5); lineTo(x + a, y + h - 5); moveTo(x + w - a, y + 5); lineTo(x + w - a, y + h - 5); strokeColor(C_LINE); stroke();
        for (int side = 0; side < 2; ++side) {
            const bool hot = presetHover == (side == 0 ? 0 : 2);
            const float cx = side == 0 ? x + a / 2 : x + w - a / 2, cy = y + h / 2, d = side == 0 ? -1.f : 1.f;
            beginPath(); moveTo(cx - 3 * d, cy - 5); lineTo(cx + 3 * d, cy); lineTo(cx - 3 * d, cy + 5); closePath(); fillColor(hot ? C_ACC : C_DIM); fill();
        }
        std::string name; bool modified = false;
        if (!curUser.empty()) { name = curUser; modified = !matchesValues(userVals); }
        else if (curFactory >= 0) { name = presets()[curFactory].name; modified = !matchesValues(presets()[curFactory].v); }
        else name = "Custom";
        if (modified) name += " *";
        fontSize(11.5f); textLetterSpacing(0.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(presetHover == 1 || menuOpen ? C_TEXT : Color(0xd2, 0xd0, 0xcc));
        text(x + w / 2, y + h / 2 + 0.5f, name.c_str(), nullptr);
    }
    // preset browser: categories, user presets and favourites in columns; typing searches names and categories.
    // kind: 0 factory, 1 user, 2 save button, 3 search field, 4 favourite (idx into favs)
    struct MRect { float x, y, w, h; int kind; int idx; };
    static std::string lower(const std::string& s) { std::string r(s); for (char& c : r) c = (char)std::tolower((unsigned char)c); return r; }
    std::string favKey(int kind, int idx) const { return kind == 0 ? std::string("f:") + presets()[idx].name : (kind == 1 ? "u:" + userPresets[idx] : (idx < (int)favs.size() ? favs[idx] : std::string())); }
    bool isFav(const std::string& k) const { return std::find(favs.begin(), favs.end(), k) != favs.end(); }
    void toggleFav(const std::string& k) {
        if (k.empty()) return;
        auto it = std::find(favs.begin(), favs.end(), k); if (it != favs.end()) favs.erase(it); else favs.push_back(k);
        saveFavourites(favs); repaint();
    }
    static constexpr int kMenuCols = kNumCategories + 2, kMenuRows = 16;
    void menuLayout(float& mx, float& my, float& mw, float& mh, std::vector<MRect>& items) const {
        mw = PM_COLW * kMenuCols + 2 * PM_PAD; mx = std::min(W - mw - 8, std::max(8.f, PS_X + PS_W / 2 - mw / 2)); my = PS_Y + PS_H + 4;
        items.clear();
        const float top = my + PM_PAD + PM_ROW + 6 + PM_HEAD;
        int maxRows = 1;
        items.push_back({ mx + PM_PAD + PM_COLW, my + PM_PAD, 300, PM_ROW, 3, 0 });
        items.push_back({ mx + mw - PM_PAD - 150, my + PM_PAD, 150, PM_ROW, 2, 0 });
        if (!query.empty()) {
            const std::string q = lower(query); int k = 0;
            auto place = [&](int kind, int idx) { const int col = k / kMenuRows, row = k % kMenuRows; if (col >= kMenuCols) return; items.push_back({ mx + PM_PAD + col * PM_COLW, top + row * PM_ROW, PM_COLW - 4, PM_ROW, kind, idx }); maxRows = std::max(maxRows, row + 1); ++k; };
            for (uint32_t i = 0; i < kNumPresets; ++i) if (lower(presets()[i].name).find(q) != std::string::npos || lower(presets()[i].category).find(q) != std::string::npos) place(0, (int)i);
            for (int u = 0; u < (int)userPresets.size(); ++u) if (lower(userPresets[u]).find(q) != std::string::npos || std::string("user").find(q) != std::string::npos) place(1, u);
        } else {
            int rows[kNumCategories] = {};
            for (uint32_t i = 0; i < kNumPresets; ++i) {
                const char* cat = presets()[i].category;
                if (std::strcmp(cat, "Init") == 0) { items.push_back({ mx + PM_PAD, my + PM_PAD, PM_COLW - 4, PM_ROW, 0, (int)i }); continue; }
                for (int c = 0; c < kNumCategories; ++c)
                    if (std::strcmp(cat, kCategories[c]) == 0) { items.push_back({ mx + PM_PAD + c * PM_COLW, top + rows[c] * PM_ROW, PM_COLW - 4, PM_ROW, 0, (int)i }); maxRows = std::max(maxRows, ++rows[c]); }
            }
            for (int u = 0; u < (int)userPresets.size() && u < kMenuRows; ++u) items.push_back({ mx + PM_PAD + kNumCategories * PM_COLW, top + u * PM_ROW, PM_COLW - 4, PM_ROW, 1, u });
            maxRows = std::max(maxRows, std::min(kMenuRows, (int)userPresets.size()));
            for (int f = 0; f < (int)favs.size() && f < kMenuRows; ++f) items.push_back({ mx + PM_PAD + (kNumCategories + 1) * PM_COLW, top + f * PM_ROW, PM_COLW - 4, PM_ROW, 4, f });
            maxRows = std::max(maxRows, std::min(kMenuRows, (int)favs.size()));
        }
        mh = PM_PAD + PM_ROW + 6 + PM_HEAD + maxRows * PM_ROW + PM_PAD;
    }
    std::string itemLabel(const MRect& r) const {
        if (r.kind == 0) return presets()[r.idx].name;
        if (r.kind == 1) return userPresets[r.idx];
        if (r.kind == 4 && r.idx < (int)favs.size()) return favs[r.idx].substr(2);
        return "";
    }
    void drawPresetMenu() {
        float mx, my, mw, mh; std::vector<MRect> items; menuLayout(mx, my, mw, mh, items);
        beginPath(); rect(mx - 12, my - 4, mw + 24, mh + 20); fillPaint(boxGradient(mx, my + 2, mw, mh, 6, 14, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(mx, my, mw, mh, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(mx + PM_PAD, my + PM_PAD + PM_ROW + 3); lineTo(mx + mw - PM_PAD, my + PM_PAD + PM_ROW + 3); strokeColor(C_LINE); stroke();
        fontSize(8.5f); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        const float hy = my + PM_PAD + PM_ROW + 6 + PM_HEAD / 2;
        if (query.empty()) {
            for (int c = 0; c < kMenuCols; ++c) {
                std::string up = c < kNumCategories ? kCategories[c] : (c == kNumCategories ? "User" : "Favourites"); for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
                fillColor(c < kNumCategories ? C_ACC : C_FAV); text(mx + PM_PAD + c * PM_COLW + 12, hy, up.c_str(), nullptr);
            }
            fontSize(10); textLetterSpacing(0.4f); fillColor(C_FAINT);
            if (userPresets.empty()) text(mx + PM_PAD + kNumCategories * PM_COLW + 12, hy + PM_HEAD / 2 + PM_ROW / 2, "(none saved yet)", nullptr);
            if (favs.empty()) text(mx + PM_PAD + (kNumCategories + 1) * PM_COLW + 12, hy + PM_HEAD / 2 + PM_ROW / 2, "(click a star)", nullptr);
        } else {
            int found = 0; for (const MRect& r : items) if (r.kind == 0 || r.kind == 1) ++found;
            char b[64]; snprintf(b, sizeof b, found ? "%d MATCHING PRESET%s" : "NO PRESET MATCHES", found, found == 1 ? "" : "S"); fillColor(C_ACC); text(mx + PM_PAD + 12, hy, b, nullptr);
        }
        for (int i = 0; i < (int)items.size(); ++i) {
            const MRect& r = items[i]; const bool hot = i == menuHover;
            if (r.kind == 2) {
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(hot ? withAlpha(C_ACC, 0.25f) : C_PANEL); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fontSize(9.5f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(r.x + r.w / 2, r.y + r.h / 2 + 0.5f, "SAVE CURRENT AS...", nullptr);
                continue;
            }
            if (r.kind == 3) {             // search field
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(C_DISPLAY); fill(); strokeColor(query.empty() ? C_LINE : C_ACC); strokeWidth(1); stroke();
                beginPath(); circle(r.x + 12, r.y + r.h / 2, 4); moveTo(r.x + 15, r.y + r.h / 2 + 3); lineTo(r.x + 18, r.y + r.h / 2 + 6); strokeColor(C_DIM); strokeWidth(1.3f); stroke();
                fontSize(11); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(query.empty() ? C_FAINT : C_TEXT);
                text(r.x + 26, r.y + r.h / 2 + 0.5f, query.empty() ? "Type to search names and categories" : (query + "|").c_str(), nullptr);
                continue;
            }
            const std::string key = favKey(r.kind, r.idx);
            const bool isCur = r.kind == 0 ? (curUser.empty() && r.idx == curFactory) : (r.kind == 1 ? curUser == userPresets[r.idx] : false);
            const bool del = r.kind == 1 && pendingDelete == r.idx;
            if (hot || del) { beginPath(); roundedRect(r.x, r.y, r.w, r.h, 4); fillColor(del ? Color(0xd0, 0x50, 0x40, 0.35f) : withAlpha(C_ACC, 0.18f)); fill(); }
            if (isCur) { beginPath(); circle(r.x + 6, r.y + r.h / 2, 2.5f); fillColor(C_ACC); fill(); }
            fontSize(11); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot || isCur || del ? C_TEXT : C_DIM);
            const std::string label = del ? "Delete? click again" : itemLabel(r);
            text(r.x + 12, r.y + r.h / 2 + 0.5f, label.c_str(), nullptr);
            // the favourite star
            const bool fav = isFav(key);
            if (fav || hot) drawStar(r.x + r.w - 10, r.y + r.h / 2, 5.f, fav);
        }
    }
    void drawStar(float cx, float cy, float rr, bool filled) {
        beginPath();
        for (int k = 0; k < 10; ++k) { const float a = -3.14159265f / 2 + k * 3.14159265f / 5, q = k % 2 ? rr * 0.45f : rr; if (!k) moveTo(cx + q * std::cos(a), cy + q * std::sin(a)); else lineTo(cx + q * std::cos(a), cy + q * std::sin(a)); }
        closePath();
        if (filled) { fillColor(C_FAV); fill(); } else { strokeColor(C_DIM); strokeWidth(1); stroke(); }
    }
    int menuHit(float x, float y) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it);
        for (int i = 0; i < (int)it.size(); ++i) if (x >= it[i].x && x < it[i].x + it[i].w && y >= it[i].y && y < it[i].y + it[i].h) return i;
        return -1; }
    MRect menuItem(int i) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it); return it[i]; }
    void openMenu() { userPresets = listUserPresets(); favs = loadFavourites(); query.clear(); menuOpen = true; menuHover = -1; pendingDelete = -1; repaint(); }
    void applyFavourite(int idx) {
        if (idx >= (int)favs.size()) return;
        const std::string& k = favs[idx];
        if (k.compare(0, 2, "f:") == 0) { for (uint32_t i = 0; i < kNumPresets; ++i) if (k.substr(2) == presets()[i].name) { applyFactory((int)i); return; } }
        else applyUser(k.substr(2));
    }
    void drawSaveBox() {
        const float w = 360, h = 96, x = W / 2 - w / 2, y = 130;
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.45f)); fill();
        beginPath(); roundedRect(x, y, w, h, 8); fillColor(C_PANEL_HI); fill(); strokeColor(C_OUT); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(x + 16, y + 14, "SAVE PRESET AS", nullptr);
        beginPath(); roundedRect(x + 16, y + 36, w - 32, 26, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); stroke();
        fontSize(13); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(x + 26, y + 49.5f, (saveName + "|").c_str(), nullptr);
        fontSize(8.5f); textLetterSpacing(1.2f); fillColor(C_FAINT); textAlign(ALIGN_RIGHT | ALIGN_TOP);
        text(x + w - 16, y + 72, saveError ? "COULD NOT SAVE - CHECK THE NAME" : "ENTER TO SAVE   ·   ESC TO CANCEL", nullptr);
    }

    // ---- interaction --------------------------------------------------------------
    static bool inRect(float x, float y, float rx, float ry, float rw, float rh) { return x >= rx && x < rx + rw && y >= ry && y < ry + rh; }
    int hit(float x, float y, int* seg = nullptr) const {
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) {
            const Ctl& c = cs[i];
            switch (c.type) {
            case Knob: { const float rr = c.r + 6; if ((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y) <= rr * rr) return i; break; }
            case Toggle: if (x >= c.x - 20 && x <= c.x + 150 && std::fabs(y - c.y) <= 12) return i; break;
            case Segment: { int n = 0; segLabels(c.param, n); const int per = (n + c.rows - 1) / c.rows; const float x0 = c.x - c.r / 2;
                const int row = (int)std::floor((y - (c.y - SEG_H / 2 - 2)) / (SEG_H + 4));
                if (x >= x0 && x < x0 + c.r && row >= 0 && row < c.rows) { const int k = row * per + (int)((x - x0) / (c.r / per)); if (k < n) { if (seg) *seg = k; return i; } } break; }
            }
        }
        return -1;
    }
    int headerHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H) return 0;
        if (x >= LEARN_X && x <= LEARN_X + LEARN_W) return 1;
        if (x >= MIDI_X && x <= MIDI_X + MIDI_W) return 2;
        if (x >= UNDO_X && x <= UNDO_X + UR_W) return 3;
        if (x >= REDO_X && x <= REDO_X + UR_W) return 4;
        if (x >= HELP_X && x <= HELP_X + HELP_W) return 5;
        if (x >= A_X && x < A_X + AB_W) return 6;
        if (x >= A_X + AB_W && x < A_X + 2 * AB_W) return 7;
        if (x >= COPY_X && x <= COPY_X + COPY_W) return 8;
        if (x >= CMP_X && x <= CMP_X + CMP_W) return 9;
        return 0;
    }
    int presetBarHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H || x < PS_X || x > PS_X + PS_W) return -1;
        return x < PS_X + PS_ARROW ? 0 : (x > PS_X + PS_W - PS_ARROW ? 2 : 1);
    }
    bool clearHit(float x, float y) const { return learnMode && learnWaiting < 0 && inRect(x, y, LX + LW - 190, LY + 8, 180, 20); }
    bool inDisplay(float x, float y) const { return !compact() && inRect(x, y, LX + 30, LY + 6, LW - 36, LH - 20); }
    bool ceilHit(float x, float y) const { return inDisplay(x, y) && std::fabs(y - yOfDb(val[kCeiling], dispTop(), dispH())) <= 6; }
    bool metersHit(float x, float y) const { return compact() ? inRect(x, y, LX, LY, 600, STRIP_H - 8) : inRect(x, y, MX, MY, 120, MH); }
    bool loudHit(float x, float y) const { return compact() ? inRect(x, y, LX + 600, LY, W - 2 * LX - 600, STRIP_H - 8) : inRect(x, y, MX + 120, MY, MW - 120, MH); }

    void setLearn(bool on) { learnMode = on; learnWaiting = -1; if (!on) setState("cmd", "cancel"); repaint(); }
    void endEdit(bool apply) {
        if (editing < 0) return;
        const uint32_t p = controls()[editing].param; float v;
        if (apply && parse(p, editBuf, v)) { beginGesture(); commit(p, v); endGesture(); }
        editing = -1; editBuf.clear(); repaint();
    }
    static float angleNorm(const Ctl& c, float x, float y) {
        float a = std::atan2(y - c.y, x - c.x);
        const float a0 = 0.75f * 3.14159265f;
        float rel = a - a0; while (rel < 0) rel += 2 * 3.14159265f;
        if (rel > 1.5f * 3.14159265f) rel = rel > 1.75f * 3.14159265f ? 0.f : 1.5f * 3.14159265f;
        return rel / (1.5f * 3.14159265f);
    }

    bool onKeyboard(const KeyboardEvent& ev) override {
        if (!ev.press) return false;
        std::string* buf = saving ? &saveName : (editing >= 0 ? &editBuf : nullptr);
        if (!buf && menuOpen) {                              // typing in the preset browser searches
            if (ev.key == kKeyEscape) { if (!query.empty()) query.clear(); else menuOpen = false; }
            else if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!query.empty()) query.pop_back(); }
            else if (ev.key >= 32 && ev.key < 127 && query.size() < 32) query += (char)ev.key;
            else return false;
            menuHover = -1; pendingDelete = -1; repaint(); return true;
        }
        if (!buf) {
            const bool cmd = (ev.mod & (kModifierControl | kModifierSuper)) != 0;
            if (cmd && (ev.key == 'z' || ev.key == 'Z')) { if (ev.mod & kModifierShift) redo(); else undo(); return true; }
            if (cmd && (ev.key == 'y' || ev.key == 'Y')) { redo(); return true; }
            if (ev.key == kKeyEscape && helpOpen) { helpOpen = false; repaint(); return true; }
            if (ev.key == kKeyEscape && learnMode) { setLearn(false); return true; }
            return false;
        }
        if (ev.key == kKeyEnter) {
            if (saving) {
                if (saveUserPreset(saveName, val)) { userPresets = listUserPresets(); curUser = sanitizeName(saveName); curFactory = -1; std::copy(val, val + kParamCount, userVals); saving = false; saveError = false; }
                else saveError = true;
            } else endEdit(true);
            repaint(); return true;
        }
        if (ev.key == kKeyEscape) { if (saving) saving = false; else endEdit(false); repaint(); return true; }
        if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!buf->empty()) buf->pop_back(); repaint(); return true; }
        if (ev.key >= 32 && ev.key < 127 && buf->size() < (saving ? 40u : 12u)) { *buf += (char)ev.key; repaint(); return true; }
        return true;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (ev.press) { statusFlash = 0; tipShown = false; tipSuppressed = tipKey; }
        if (helpOpen) { if (ev.press && ev.button == 1) helpClick(x, y); return true; }
        if (ev.button == 2 && ev.press) {
            if (menuOpen) { const int m = menuHit(x, y); if (m >= 0 && menuItem(m).kind == 1) { pendingDelete = menuItem(m).idx; repaint(); } return true; }
            if (learnMode) { const int i = hit(x, y); if (i >= 0) { const uint32_t p = controls()[i].param; char b[24]; snprintf(b, sizeof b, "unmap:%u", p); setState("cmd", b); paramCC[p] = -1; repaint(); } return true; }
            return false;
        }
        if (ev.button != 1) return false;
        if (!ev.press) {
            if (active >= 0) { editParameter(controls()[active].param, false); active = -1; endGesture(); return true; }
            if (dragCeil) { editParameter(kCeiling, false); dragCeil = false; endGesture(); return true; }
            if (dragGain) { editParameter(kGain, false); dragGain = false; endGesture(); return true; }
            return false;
        }
        if (editing >= 0) endEdit(true);
        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m >= 0) {
                const MRect r = menuItem(m);
                if (r.kind == 3) return true;                       // the search field: just type
                if ((r.kind == 0 || r.kind == 1 || r.kind == 4) && x >= r.x + r.w - 20) { toggleFav(favKey(r.kind, r.idx)); return true; }
                if (r.kind == 2) { menuOpen = false; saving = true; saveError = false; saveName = curUser; }
                else if (r.kind == 1 && pendingDelete == r.idx) { deleteUserPreset(userPresets[r.idx]); if (curUser == userPresets[r.idx]) curUser.clear(); userPresets = listUserPresets(); pendingDelete = -1; repaint(); return true; }
                else { menuOpen = false; if (r.kind == 0) applyFactory(r.idx); else if (r.kind == 1) applyUser(userPresets[r.idx]); else applyFavourite(r.idx); }
            } else menuOpen = false;
            pendingDelete = -1; menuHover = -1; repaint(); return true;
        }
        const int hh = headerHit(x, y);
        if (hh == 1) { setLearn(!learnMode); return true; }
        if (hh == 2) { beginGesture(); commit(kMidiOn, val[kMidiOn] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
        if (hh == 3) { undo(); return true; }
        if (hh == 4) { redo(); return true; }
        if (hh == 5) { helpOpen = true; repaint(); return true; }
        if (hh == 6 || hh == 7) { switchAB(hh - 6); return true; }
        if (hh == 8) { copyAB(); return true; }
        if (hh == 9) { commit(kCompact, compact() ? 0.f : 1.f); return true; }
        const int pb = presetBarHit(x, y);
        if (pb == 0) { step(-1); return true; }
        if (pb == 2) { step(1); return true; }
        if (pb == 1) { openMenu(); return true; }
        if (clearHit(x, y)) { setState("cmd", "clear"); for (int& m : paramCC) m = -1; repaint(); return true; }
        if (metersHit(x, y)) { resetPeaks(); repaint(); return true; }
        if (loudHit(x, y)) { setState("cmd", "loudreset"); flash("Loudness reset"); return true; }

        int seg = 0; const int i = hit(x, y, &seg);
        if (i >= 0) {
            const Ctl& c = controls()[i];
            const uint32_t p = c.param;
            if (learnMode) { learnWaiting = (int)p; char b[24]; snprintf(b, sizeof b, "learn:%u", p); setState("cmd", b); repaint(); return true; }
            if (c.type == Toggle) { beginGesture(); commit(p, val[p] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
            if (c.type == Segment) { beginGesture(); commit(p, (float)seg); endGesture(); return true; }
            if (ev.mod & (kModifierControl | kModifierSuper)) { beginGesture(); commit(p, defv[p]); endGesture(); return true; }
            if (i == lastClick && ev.time - lastClickTime < 350) {
                lastClick = -1; editing = i;
                char b[40]; fmt(p, b, sizeof b); editBuf = b;
                const size_t sp = editBuf.find(' '); if (sp != std::string::npos) editBuf.resize(sp);
                if (!editBuf.empty() && editBuf.back() == '%') editBuf.pop_back();
                repaint(); return true;
            }
            lastClick = i; lastClickTime = ev.time;
            beginGesture();
            active = i; dragY = y; dragNorm = toNorm(p, val[p]);
            const float dist = std::sqrt((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y));
            circular = dist > c.r * 0.62f;
            editParameter(p, true);
            if (circular) setValue(p, fromNorm(p, angleNorm(c, x, y)));
            repaint(); return true;
        }
        if (!learnMode && ceilHit(x, y)) { beginGesture(); dragCeil = true; editParameter(kCeiling, true); return true; }
        if (!learnMode && inDisplay(x, y)) { beginGesture(); dragGain = true; dragY = y; dragStart = val[kGain]; editParameter(kGain, true); repaint(); return true; }
        return false;
    }

    bool onMotion(const MotionEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (helpOpen) { const int h = helpHit(x, y); if (h != helpHover) { helpHover = h; repaint(); } return true; }
        if (active >= 0) {
            const Ctl& c = controls()[active]; const uint32_t p = c.param;
            if (circular) { setValue(p, fromNorm(p, angleNorm(c, x, y))); return true; }
            const float sens = (ev.mod & kModifierShift) ? 1500.f : 240.f;
            dragNorm = clampf(dragNorm + (dragY - y) / sens, 0.f, 1.f); dragY = y;
            setValue(p, fromNorm(p, dragNorm));
            return true;
        }
        if (dragCeil) { setValue(kCeiling, std::round(clampf(dbOfY(y, dispTop(), dispH()), -30.f, 0.f) * 10.f) / 10.f); return true; }
        if (dragGain) {
            const float k = (ev.mod & kModifierShift) ? 0.2f : 1.f, dbPerPx = (headDb() + rangeDb()) / dispH();
            setValue(kGain, std::round(clampf(dragStart + (dragY - y) * dbPerPx * k, 0.f, 30.f) * 10.f) / 10.f); return true;
        }
        if (menuOpen) { const int m = menuHit(x, y); if (m != menuHover) { menuHover = m; repaint(); } return true; }
        const int hh = headerHit(x, y); if (hh != hoverHeader) { hoverHeader = hh; repaint(); }
        const int pb = presetBarHit(x, y); if (pb != presetHover) { presetHover = pb; repaint(); }
        const bool hc = clearHit(x, y); if (hc != hoverClear) { hoverClear = hc; repaint(); }
        int segAt = -1; const int h = hit(x, y, &segAt); if (h != hover) { hover = h; repaint(); }
        if (h >= 0 && controls()[h].param == kStyle && segAt != tipSeg) { tipSeg = segAt; tipKey = -1; }      // each style button has its own tip
        else if (h < 0 || controls()[h].param != kStyle) tipSeg = -1;
        const bool hcl = h < 0 && !learnMode && ceilHit(x, y); if (hcl != hoverCeil) { hoverCeil = hcl; repaint(); }
        const int key = h >= 0 ? h : (hh > 0 ? 1000 + hh : (pb >= 0 ? 2000 + pb : (hcl ? 3001 : (inDisplay(x, y) ? 3000 : (metersHit(x, y) ? 3002 : (loudHit(x, y) ? 3003 : -1))))));
        tipX = x; tipY = y;
        if (key != tipKey) { const bool was = tipShown; tipKey = key; tipSince = nowSec(); tipShown = false; if (key != tipSuppressed) tipSuppressed = -2; if (was) repaint(); }
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const float s = uiScale(), sx = ev.pos.getX() / s, sy = ev.pos.getY() / s;
        if (menuOpen || saving || helpOpen) return true;
        if (presetBarHit(sx, sy) >= 0) { step(ev.delta.getY() > 0 ? -1 : 1); return true; }
        const float st = (ev.mod & kModifierShift) ? 0.004f : 0.02f, d = ev.delta.getY() > 0 ? st : -st;
        const int i = hit(sx, sy);
        if (i < 0 || controls()[i].type != Knob) {
            if (inDisplay(sx, sy)) { beginGesture(); commit(kGain, std::round(clampf(val[kGain] + (d > 0 ? 0.5f : -0.5f) * ((ev.mod & kModifierShift) ? 0.2f : 1.f), 0.f, 30.f) * 10.f) / 10.f); endGesture(); return true; }
            return false;
        }
        const uint32_t p = controls()[i].param;
        beginGesture(); commit(p, fromNorm(p, toNorm(p, val[p]) + d)); endGesture();
        return true;
    }

    void uiIdle() override {
        const double t = nowSec();
        if (t - lastPoll > 0.03) {
            lastPoll = t;
            if (GranitePlugin* const dsp = (GranitePlugin*)getPluginInstancePointer()) {
                const Engine& e = dsp->engine;
                const uint32_t w = e.histWrite.load();
                uint32_t from = lastHist; if (w - from > (uint32_t)HIST || w < from) from = w > (uint32_t)HIST ? w - HIST : 0;
                for (uint32_t k = from; k < w; ++k) {
                    const uint32_t r = k % Engine::kHist;
                    hIn[hPos] = e.hIn[r]; hOut[hPos] = e.hOut[r]; hGr[hPos] = e.hGr[r]; hRms[hPos] = e.hRms[r]; hLoud[hPos] = e.hLoud[r];
                    hPos = (hPos + 1) % HIST;
                }
                lastHist = w;
                latencyMs = e.latency() * 1000.f / std::max(1.f, (float)getSampleRate());
            }
            repaint();
        }
        if (gTipsOn && !tipShown && tipKey >= 0 && tipKey != tipSuppressed && active < 0 && !dragCeil && !dragGain && !menuOpen && !saving && !helpOpen && editing < 0
            && t - tipSince > 0.6) { tipShown = true; repaint(); }
    }

    // ---- hover tips ---------------------------------------------------------------
    bool tipContent(std::string& title, std::string& value, std::string& body) const {
        if (tipKey < 0) return false;
        if (tipKey < 1000) {
            const uint32_t p = controls()[tipKey].param; char b[40]; fmt(p, b, sizeof b);
            title = paramDef(p).name; value = b; body = paramTip(p);
            if (p == kStyle && tipSeg >= 0) { title = std::string("Style: ") + enumLabelOf(kStyle, tipSeg); value.clear(); body = styleTip(tipSeg); }
            if (controls()[tipKey].type == Knob) body += "\n\nDrag the middle up/down or grab the rim and turn. Double-click to type a value, Cmd/Ctrl-click to reset.";
            if (paramCC[p] >= 0) { char cc[32]; snprintf(cc, sizeof cc, "   ·   CC%d", paramCC[p]); value += cc; }
            return true;
        }
        int t = -1;
        if (tipKey >= 3000) t = tipKey == 3000 ? kTipDisplay : (tipKey == 3001 ? kTipCeilingLine : (tipKey == 3002 ? kTipMeters : kTipLoudness));
        else if (tipKey >= 2000) t = kTipPrev + (tipKey - 2000);
        else { const int hh = tipKey - 1000;
            t = hh == 1 ? kTipLearn : hh == 2 ? kTipMidi : hh == 3 ? kTipUndo : hh == 4 ? kTipRedo : hh == 5 ? kTipHelp : hh == 6 ? kTipA : hh == 7 ? kTipB : hh == 8 ? kTipCopy : kTipCompact; }
        title = targetTitle(t); value.clear(); body = targetTip(t);
        if (t == kTipUndo || t == kTipRedo) { const size_t n = t == kTipUndo ? undoStack.size() : redoStack.size(); char b[32]; snprintf(b, sizeof b, "%d step%s", (int)n, n == 1 ? "" : "s"); value = b; }
        if (t == kTipCeilingLine) { char b[24]; fmt(kCeiling, b, sizeof b); value = b; }
        if (t == kTipDisplay) { char b[24]; fmt(kGain, b, sizeof b); value = std::string("gain ") + b; }
        return true;
    }
    void drawTooltip() {
        std::string title, value, body;
        if (!tipContent(title, value, body)) return;
        const float w = 310, pad = 12;
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f);
        float bb[4]; textBoxBounds(0, 0, w - 2 * pad, body.c_str(), nullptr, bb);
        const float h = pad + 16 + 8 + (bb[3] - bb[1]) + pad;
        float x = tipX + 16, y = tipY + 20;
        if (x + w > W - 8) x = tipX - w - 12;
        if (y + h > H - 8) y = tipY - h - 12;
        x = clampf(x, 8, W - w - 8); y = clampf(y, 8, H - h - 8);
        beginPath(); rect(x - 10, y - 4, w + 20, h + 18); fillPaint(boxGradient(x, y + 3, w, h, 6, 14, Color(0, 0, 0, 0.55f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(Color(0x1f, 0x1f, 0x22)); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_TEXT);
        std::string up = title; for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
        text(x + pad, y + pad + 1, up.c_str(), nullptr);
        if (!value.empty()) { fontSize(10.5f); textLetterSpacing(0.4f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_ACC); text(x + w - pad, y + pad + 1, value.c_str(), nullptr); }
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM);
        textBox(x + pad, y + pad + 24, w - 2 * pad, body.c_str(), nullptr);
        textLineHeight(1.f);
    }

    // ---- help window ---------------------------------------------------------------
    static constexpr float HX = (W - 760) / 2, HW = 760, TAB_W = 180, TAB_H = 34;
    float hy() const { return compact() ? 10.f : 70.f; }
    float hh() const { return compact() ? HC - 20 : 520.f; }
    int helpHit(float x, float y) const {
        if (x < HX || x > HX + HW || y < hy() || y > hy() + hh()) return -2;
        for (int i = 0; i < kNumHelpPages; ++i) if (x >= HX + 12 && x < HX + 12 + TAB_W && y >= hy() + 56 + i * TAB_H && y < hy() + 56 + (i + 1) * TAB_H) return i;
        const float by = hy() + hh() - 44;
        if (y >= by && y < by + 28) {
            if (x >= HX + 12 && x < HX + 12 + TAB_W) return 100;
            if (x >= HX + HW - 330 && x < HX + HW - 140) return 101;
            if (x >= HX + HW - 124 && x < HX + HW - 16) return 102;
        }
        if (x >= HX + HW - 40 && y < hy() + 40) return 102;
        return -1;
    }
    void helpClick(float x, float y) {
        const int h = helpHit(x, y);
        if (h >= 0 && h < kNumHelpPages) helpPage = h;
        else if (h == 100) gTipsOn = !gTipsOn;
        else if (h == 101) openURL(kManualURL);
        else if (h == 102 || h == -2) helpOpen = false;
        repaint();
    }
    void drawHelp() {
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.55f)); fill();
        beginPath(); roundedRect(HX, hy(), HW, hh(), 10); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(13); textLetterSpacing(4.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(HX + 22, hy() + 28, "GRANITE HELP", nullptr);
        const float cx = HX + HW - 24, cy = hy() + 26;
        beginPath(); moveTo(cx - 6, cy - 6); lineTo(cx + 6, cy + 6); moveTo(cx + 6, cy - 6); lineTo(cx - 6, cy + 6); strokeColor(helpHover == 102 ? C_TEXT : C_DIM); strokeWidth(1.6f); stroke();
        for (int i = 0; i < kNumHelpPages; ++i) {
            const float ty = hy() + 56 + i * TAB_H;
            if (i == helpPage || i == helpHover) { beginPath(); roundedRect(HX + 12, ty + 2, TAB_W, TAB_H - 4, 5); fillColor(i == helpPage ? withAlpha(C_ACC, 0.16f) : C_PANEL_HI); fill(); }
            if (i == helpPage) { beginPath(); roundedRect(HX + 12, ty + 8, 3, TAB_H - 16, 1.5f); fillColor(C_ACC); fill(); }
            fontSize(11.5f); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(i == helpPage ? C_TEXT : C_DIM);
            text(HX + 28, ty + TAB_H / 2 + 0.5f, kHelpPages[i].title, nullptr);
        }
        beginPath(); moveTo(HX + TAB_W + 26, hy() + 56); lineTo(HX + TAB_W + 26, hy() + hh() - 56); strokeColor(C_LINE); strokeWidth(1); stroke();
        const float bx = HX + TAB_W + 46, bw = HW - TAB_W - 70;
        fontSize(15); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_ACC); text(bx, hy() + 60, kHelpPages[helpPage].title, nullptr);
        fontSize(12); textLetterSpacing(0.2f); textLineHeight(1.38f); fillColor(Color(0xc6, 0xc4, 0xc0));
        textBox(bx, hy() + 92, bw, kHelpPages[helpPage].body, nullptr);
        textLineHeight(1.f);
        const float by = hy() + hh() - 44;
        beginPath(); moveTo(HX + 12, by - 10); lineTo(HX + HW - 12, by - 10); strokeColor(C_LINE); stroke();
        const float tw = 34, th = 18, tx = HX + 16, tyy = by + 14 - th / 2;
        beginPath(); roundedRect(tx, tyy, tw, th, th / 2); fillColor(gTipsOn ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(gTipsOn ? C_ACC : C_LINE); stroke();
        beginPath(); circle(gTipsOn ? tx + tw - th / 2 : tx + th / 2, by + 14, 6); fillColor(gTipsOn ? C_ACC : C_DIM); fill();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(helpHover == 100 ? C_TEXT : C_DIM); text(tx + tw + 10, by + 14.5f, "HOVER TIPS", nullptr);
        auto btn = [&](float x0, float w0, const char* label, bool hot, bool primary) {
            beginPath(); roundedRect(x0, by, w0, 28, 5); fillColor(primary ? (hot ? withAlpha(C_ACC, 0.32f) : withAlpha(C_ACC, 0.16f)) : (hot ? C_PANEL_HI : C_PANEL)); fill();
            strokeColor(primary ? C_ACC : C_LINE); strokeWidth(1); stroke();
            fontSize(9.5f); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(x0 + w0 / 2, by + 14.5f, label, nullptr);
        };
        btn(HX + HW - 330, 190, "FULL MANUAL (WEB)", helpHover == 101, false);
        btn(HX + HW - 124, 108, "CLOSE", helpHover == 102, true);
    }

private:
    float val[kParamCount], defv[kParamCount], userVals[kParamCount] = {}, before[kParamCount];
    int paramCC[kParamCount];
    int active = -1, hover = -1, lastClick = -1, editing = -1;
    uint lastClickTime = 0;
    float dragY = 0, dragNorm = 0, dragStart = 0; bool circular = false, gesture = false;
    bool dragCeil = false, hoverCeil = false, dragGain = false;
    std::vector<Snap> undoStack, redoStack;
    Snap abStore[2]; int abSlot = 0;
    std::string editBuf, statusText; int statusFlash = 0;
    int curFactory = 0, presetHover = -1, menuHover = -1, pendingDelete = -1, hoverHeader = 0;
    std::string curUser, query; std::vector<std::string> userPresets, favs;
    bool menuOpen = false, saving = false, saveError = false; std::string saveName;
    bool learnMode = false, hoverClear = false; int learnWaiting = -1; float lastLearnEcho = 0;
    int tipKey = -1, tipSuppressed = -2, tipSeg = -1; double tipSince = 0; bool tipShown = false; float tipX = 0, tipY = 0;
    bool helpOpen = false; int helpPage = 0, helpHover = -1;
    float hIn[HIST], hOut[HIST], hGr[HIST], hRms[HIST], hLoud[HIST]; int hPos = 0; uint32_t lastHist = 0; double lastPoll = 0;
    float pkIn = -120, pkOut = -120, pkGr = 0, latencyMs = 0;
    ResizeHandle resizeHandle;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraniteUI)
};

UI* createUI() { return new GraniteUI(); }

END_NAMESPACE_DISTRHO
