// Granite — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "GraniteEngine.hpp"

namespace granite {

static const char* const kManualURL = "https://github.com/natehouk/granite/blob/main/docs/MANUAL.md";

inline const char* paramTip(uint32_t p) {
    switch (p) {
    case kGain: return "How hard you push the signal into the limiter. Every dB of gain above the output level becomes a dB of gain reduction (the red area in the display) and a dB of loudness.";
    case kCeiling: return "The output level: no sample will go above it. With oversampling at 2x or 4x it also catches peaks that form between samples (true peaks). -1 dB is a safe choice for streaming. You can also drag the dashed line in the display.";
    case kStyle: return "The limiter's character. CLEAR: transparent, slows its release during sustained limiting to avoid pumping. PUNCHY: shorter attack and faster release, keeps drums snappy. DYNAMIC: recovers quickly after deep peaks, keeps more movement. BALANCED: rounds peaks off gently before limiting, a little color and less reduction.";
    case kLookahead: return "How far ahead the limiter looks, 0 to 10 ms, so it can turn the gain down smoothly before a peak arrives. Longer is cleaner on fast transients but adds that much latency (the host compensates). 0 ms turns Granite into a clipper.";
    case kAttack: return "How long the gain takes to come down ahead of a peak (never longer than the look-ahead). Long attack is smooth and transparent; short attack keeps transients sharper but can add distortion.";
    case kRelease: return "How quickly the gain recovers after a peak. Fast release is louder but can distort or pump on bass; slow release is smoother but holds the level down longer.";
    case kLinkTransient: return "Channel linking for the fast (transient) stage. 100%: both channels are turned down together, so the stereo image stays put. 0%: each channel is limited on its own, which can be louder but may shift the image on one-sided peaks.";
    case kLinkRelease: return "Channel linking for the recovery (release) stage. Linked release keeps the image stable while the gain recovers; unlinked release lets each side recover on its own.";
    case kOversampling: return "Runs the limiter at 2x or 4x the sample rate with linear-phase filters. This reduces aliasing distortion and limits inter-sample (true) peaks as well as sample peaks. Costs a little CPU and adds 32 or 48 samples of latency.";
    case kTruePeak: return "When on, the output meters estimate the true (inter-sample) peak, the level a D/A converter or a lossy codec will actually see. Off shows the sample peak.";
    case kDither: return "Adds a tiny amount of noise before reducing the output to 16, 20 or 24 bits, so quiet passages fade smoothly instead of turning to distortion. Use it only on the final master, as the very last step, at the bit depth you export.";
    case kShaping: return "Moves the dither noise to where the ear is least sensitive. FLAT: plain dither. BASIC: gentle high-frequency tilt. OPTIMIZED and WEIGHTED: shaped to hearing curves, the quietest-sounding at 44.1 and 48 kHz.";
    case kMeterScale: return "The meters' scale: dBFS, or the K-System (K-20, K-14, K-12), which puts 0 at an average level 20, 14 or 12 dB below full scale. Display only; it does not change the sound.";
    case kDisplayRange: return "How many dB the level display and meters show. 12 dB zooms in on the top of the signal for fine mastering work; 48 dB shows the whole picture.";
    case kMidiOn: return "Turns MIDI control on or off. When off, learned controllers are ignored.";
    default: return "";
    }
}

enum TipTarget { kTipUndo, kTipRedo, kTipLearn, kTipMidi, kTipHelp, kTipA, kTipB, kTipCopy, kTipPrev, kTipPresets, kTipNext,
                 kTipDisplay, kTipCeilingLine, kTipMeters, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "MIDI", "Help", "Setting A", "Setting B", "Copy A/B",
                                                   "Previous preset", "Presets", "Next preset", "Level display", "Output level", "Meters" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including preset loads and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Right-click a control to unassign.";
    case kTipMidi: return paramTip(kMidiOn);
    case kTipHelp: return "Opens the help pages: getting loud safely, styles, look-ahead, attack and release, linking, oversampling and true peaks, dither, metering, A/B and MIDI Learn.";
    case kTipA: case kTipB: return "Granite keeps two complete settings, A and B. Click the other letter to switch and compare by ear. Match the loudness first, or louder will always sound better.";
    case kTipCopy: return "Copies the current setting to the other slot.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser: mastering, genre, mix-bus and utility presets, your own presets, and Save Current As. A * after the name means you have changed it.";
    case kTipNext: return "Load the next preset.";
    case kTipDisplay: return "The last ten seconds, scrolling left. Grey: the signal going in (after Gain). Light: the output. Red: where the limiter is working, its depth is the gain reduction. The thin line is the output's RMS (average) level. Drag up and down to change Gain; the mouse wheel does the same.";
    case kTipCeilingLine: return "The output level. Drag it up or down. No output sample will cross it.";
    case kTipMeters: return "IN: the level going into the limiter (after Gain). GR: gain reduction right now. OUT: the output level, as a true-peak estimate when True Peak is on. The numbers are the peak values; click the meters to reset them.";
    default: return "";
    }
}

struct HelpPage { const char* title; const char* body; };
static const HelpPage kHelpPages[] = {
    { "Overview",
      "Granite is a brickwall limiter: it makes audio louder while guaranteeing that nothing goes above the output level you set.\n\n"
      "Turn up GAIN to push the signal into the limiter, and set OUTPUT LEVEL to the highest level you want to allow (for example -1 dB for streaming). "
      "The red area in the display shows how much the limiter is working. A few dB is usually transparent; much more starts to flatten the music.\n\n"
      "Hover over anything for a moment to see what it does. Turn the tips off at the bottom of this window." },
    { "Styles",
      "CLEAR is the most transparent: when it has to limit for a while, it lengthens its release so the level does not pump.\n\n"
      "PUNCHY uses a shorter attack and half the release, which keeps drums and percussion snappy and loud.\n\n"
      "DYNAMIC recovers faster the deeper it has had to limit, so short peaks leave little trace and the music keeps its movement.\n\n"
      "BALANCED rounds the very top of the peaks off with soft saturation before limiting: a touch of warmth and less gain reduction for the same loudness." },
    { "Look-ahead, attack, release",
      "LOOK-AHEAD (0 to 10 ms) lets Granite see peaks coming and lower the gain smoothly beforehand. It adds the same amount of latency, which your DAW compensates. At 0 ms Granite acts as a clipper.\n\n"
      "ATTACK sets how gradually the gain comes down ahead of a peak, up to the look-ahead time. Longer is smoother; shorter keeps more transient snap.\n\n"
      "RELEASE sets how quickly the gain recovers. Short release is louder but can distort bass and pump; long release is smoother.\n\n"
      "The limit is always exact: whatever you set, the output never goes over the output level." },
    { "Linking & oversampling",
      "CHANNEL LINK has two knobs. TRANSIENT links the fast gain changes between left and right, RELEASE links the recovery. Fully linked keeps the stereo image rock steady; "
      "less linking can be louder, but a loud event on one side will then move the image.\n\n"
      "OVERSAMPLING (2x or 4x) runs the limiter at a higher rate with linear-phase filters. It reduces aliasing and also catches inter-sample peaks: "
      "peaks that appear between samples after D/A conversion or lossy encoding. For masters that go to streaming, use 4x with an output level of -1 dB." },
    { "Dither & metering",
      "DITHER adds a very small, carefully shaped noise when the output is reduced to 16, 20 or 24 bits, so fades and quiet passages stay smooth. Use it only once, as the last "
      "step of a final master, at the bit depth you are exporting. Noise shaping moves that noise to frequencies where it is hardest to hear.\n\n"
      "METERS show input, gain reduction and output. TRUE PEAK makes the output meter estimate inter-sample peaks. METER SCALE switches to the K-System (K-20, K-14, K-12), "
      "and DISPLAY RANGE zooms the display and meters to the top 12, 24 or 48 dB." },
    { "A/B, undo & MIDI",
      "A/B holds two complete settings; COPY copies the current one to the other slot. When comparing, match the loudness, because louder always sounds better.\n\n"
      "Every edit is an undo step. Cmd/Ctrl-Z undoes, Cmd/Ctrl-Shift-Z redoes.\n\n"
      "MIDI LEARN: click LEARN, click a control, move a knob on your controller. Right-click to unassign; LEARN again or Esc to finish.\n\n"
      "Double-click a knob to type a value; Cmd/Ctrl-click resets it. Shift-drag is fine adjustment. Esc closes this window." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace granite
