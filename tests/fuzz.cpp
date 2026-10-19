// Random parameters (including structural changes) at 44.1-192 kHz with loud noise:
// output always finite and never above the output level in force.
#include "../plugins/Granite/ParamDefs.hpp"
#include <cstdio>
#include <random>
#include <vector>
using namespace granite;
int main() {
    std::mt19937 g(17); std::uniform_real_distribution<float> u(0, 1); int bad = 0; float worstOver = -100;
    for (int sr : { 44100, 48000, 96000, 192000 }) {
        Engine e; e.setSampleRate((float)sr);
        // the output level glides over ~20 ms: allow the highest value set within the last 250 ms
        std::vector<float> recent; const size_t keep = (size_t)(0.25 * sr / 400) + 2;
        for (int it = 0; it < 1500; ++it) {
            for (uint32_t i = 0; i < kParamCount; ++i) {
                const ParamDef& d = paramDef(i); if (d.hints & kHintOutput) continue;
                if (u(g) < 0.7f) continue;
                float v = (u(g) < .3f) ? (u(g) < .5f ? d.min : d.max) : d.min + (d.max - d.min) * u(g);
                if (d.hints & kHintInteger) v = std::round(v);
                e.p[i] = v;
            }
            float in[400], L[400], R[400]; const float amp = u(g) < .2f ? 0 : (u(g) < .5f ? 8.f : 0.7f);
            for (float& x : in) x = amp * (2 * u(g) - 1);
            e.process(in, in, L, R, 400);
            // the output level glides over ~20 ms, so allow the larger of the old and new value
            recent.push_back(e.p[kCeiling]); if (recent.size() > keep) recent.erase(recent.begin());
            float lim = -100; for (float c : recent) lim = std::max(lim, c);
            for (int k = 0; k < 400; ++k) {
                if (!std::isfinite(L[k]) || !std::isfinite(R[k])) bad++;
                const float over = gainToDb(std::max(std::fabs(L[k]), std::fabs(R[k]))) - lim;
                worstOver = std::max(worstOver, over);
            }
        }
    }
    printf("fuzz nonfinite=%d worst over the output level %+.3f dB \n", bad, worstOver);
    return bad || worstOver > 0.001f;
}
