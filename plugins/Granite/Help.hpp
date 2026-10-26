// Granite — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "GraniteEngine.hpp"

namespace granite {

static const char* const kManualURL = "https://github.com/natehouk/granite/blob/main/docs/MANUAL.md";

inline const char* paramTip(uint32_t p) {
    switch (p) {
    case kGain: return "How hard you push the signal into the limiter. Every dB of gain above the output level becomes a dB of gain reduction (the red area in the display) and a dB of loudness.";
    case kCeiling: return "The output level: no sample will go above it. Turn on TRUE PEAK LIMITING to keep the peaks between samples (true peaks) under it too. -1 dB is a safe choice for streaming. You can also drag the dashed line in the display.";
    case kStyle: return "The limiter's character: eight styles, from the transparent CLEAR and SAFE to the loud AGGRESSIVE. Hover over each button for what it does.";
    case kLookahead: return "How far ahead the limiter looks, 0 to 10 ms, so it can turn the gain down smoothly before a peak arrives. Longer is cleaner on fast transients but adds that much latency (the host compensates). 0 ms turns Granite into a clipper.";
    case kAttack: return "How long the gain takes to come down ahead of a peak (never longer than the look-ahead). Long attack is smooth and transparent; short attack keeps transients sharper but can add distortion.";
    case kRelease: return "How quickly the gain recovers after a peak. Fast release is louder but can distort or pump on bass; slow release is smoother but holds the level down longer.";
    case kLinkTransient: return "Channel linking for the fast (transient) stage. 100%: both channels are turned down together, so the stereo image stays put. 0%: each channel is limited on its own, which can be louder but may shift the image on one-sided peaks.";
    case kLinkRelease: return "Channel linking for the recovery (release) stage. Linked release keeps the image stable while the gain recovers; unlinked release lets each side recover on its own.";
    case kOversampling: return "Runs the limiter at 2x to 32x the sample rate with linear-phase filters. This reduces aliasing distortion and catches most inter-sample peaks. Adds 32, 48, 56, 60 or 62 samples of latency; 16x and 32x cost noticeably more CPU.";
    case kTruePeakLimit: return "Limits the true (inter-sample) peak of the output, not just the samples: what a D/A converter or a lossy codec will see stays under the output level. Adds about 1 ms of latency. Use it for anything going to streaming or broadcast.";
    case kSidechain: return "INTERNAL: the limiter reacts to the audio itself. EXTERNAL: it reacts to the sidechain input instead, so a stem can be limited exactly as the full mix on the sidechain is. In External mode the audio itself is not clamped to the output level.";
    case kDcFilter: return "Removes DC offset (a constant shift away from zero) before limiting, with a gentle 5 Hz high-pass. DC wastes headroom and makes the limiter work on one side only.";
    case kUnityGain: return "Monitoring only: turns the output down by the GAIN amount, so you hear what the limiting does to the sound at the level you started from, without being fooled by loudness. Turn off before you export.";
    case kAudition: return "Monitoring only: you hear only what the limiter removes (the input minus the output). Useful to find out when limiting starts to bite into the music. Turn off before you export.";
    case kLoudTarget: return "Shows a loudness target in the display and colours the integrated loudness green when it is within 1 LU: EBU R128 (-23 LUFS), ATSC A/85 (-24 LKFS), streaming (-14 LUFS) or podcast (-16 LUFS). The display then plots momentary loudness instead of RMS. Display only.";
    case kDisplaySpeed: return "How much history the display shows: NORMAL 10 seconds, FAST 5, SLOW 30, or OFF to show the meters only. Display only.";
    case kCompact: return "Switches between the full window and a compact one with a meter strip instead of the level display.";
    case kTruePeak: return "When on, the output meters estimate the true (inter-sample) peak, the level a D/A converter or a lossy codec will actually see. Off shows the sample peak.";
    case kDither: return "Adds a tiny amount of noise before reducing the output to 16, 20 or 24 bits, so quiet passages fade smoothly instead of turning to distortion. Use it only on the final master, as the very last step, at the bit depth you export.";
    case kShaping: return "Moves the dither noise to where the ear is least sensitive. FLAT: plain dither. BASIC: gentle high-frequency tilt. OPTIMIZED and WEIGHTED: shaped to hearing curves, the quietest-sounding at 44.1 and 48 kHz.";
    case kMeterScale: return "The meters' scale: dBFS, or the K-System (K-20, K-14, K-12), which puts 0 at an average level 20, 14 or 12 dB below full scale. Display only; it does not change the sound.";
    case kDisplayRange: return "How many dB the level display and meters show. 12 dB zooms in on the top of the signal for fine mastering work; 48 dB shows the whole picture.";
    case kMidiOn: return "Turns MIDI control on or off. When off, learned controllers are ignored.";
    default: return "";
    }
}

inline const char* styleTip(int s) {
    switch (s) {
    case kClear: return "Transparent. When it has to limit for a while, it slows its release so the level does not pump. A good first choice for mastering.";
    case kPunchy: return "A shorter attack and half the release: keeps drums and percussion snappy and loud.";
    case kDynamic: return "Recovers faster the deeper it has had to limit, so short peaks leave little trace and the music keeps its movement.";
    case kBalanced: return "Rounds the very top of the peaks off with soft saturation before limiting: a touch of warmth and less gain reduction for the same loudness.";
    case kAggressive: return "The loudest style: very fast attack and release, with harder saturation of the peaks. Adds density and grit; listen for distortion on bass.";
    case kModern: return "Adapts to the material: recovers quickly from short peaks and slows down during sustained limiting. Loud and clean on most modern mixes.";
    case kBus: return "Gentle and glueing: a longer release that lengthens further during sustained limiting. For buses and stems rather than final loudness.";
    case kSafe: return "Avoids distortion above all: always uses the full look-ahead and never releases faster than 40 ms. It may pump a little more instead.";
    default: return "";
    }
}

enum TipTarget { kTipUndo, kTipRedo, kTipLearn, kTipMidi, kTipHelp, kTipA, kTipB, kTipCopy, kTipPrev, kTipPresets, kTipNext,
                 kTipDisplay, kTipCeilingLine, kTipMeters, kTipLoudness, kTipCompact, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "MIDI", "Help", "Setting A", "Setting B", "Copy A/B",
                                                   "Previous preset", "Presets", "Next preset", "Level display", "Output level", "Meters", "Loudness", "Compact view" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including preset loads and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Right-click a control to unassign.";
    case kTipMidi: return paramTip(kMidiOn);
    case kTipHelp: return "Opens the help pages: getting loud safely, styles, look-ahead, attack and release, linking, oversampling, true peaks and loudness, sidechain and monitoring, dither, metering, A/B and MIDI Learn.";
    case kTipA: case kTipB: return "Granite keeps two complete settings, A and B. Click the other letter to switch and compare by ear. Match the loudness first, or louder will always sound better.";
    case kTipCopy: return "Copies the current setting to the other slot.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser: mastering, genre, mix-bus and utility presets, your own presets, and Save Current As. A * after the name means you have changed it. In the browser, type to search names and categories, and click a star to keep a preset in Favourites.";
    case kTipNext: return "Load the next preset.";
    case kTipDisplay: return "The last few seconds (see DISPLAY SPEED), scrolling left. Grey: the signal going in (after Gain). Light: the output. Red: where the limiter is working; its depth is the gain reduction, and the labels mark the deepest moments. The thin line is the output's RMS level, or its momentary loudness when a loudness target is set. Drag up and down to change Gain; the mouse wheel does the same.";
    case kTipCeilingLine: return "The output level. Drag it up or down. No output sample will cross it.";
    case kTipMeters: return "IN: the level going into the limiter (after Gain). GR: gain reduction right now. OUT: the output level, as a true-peak estimate when True Peak Meters is on. The numbers are the peak values; click the meters to reset them.";
    case kTipLoudness: return "Loudness of the output to ITU-R BS.1770 / EBU R128. M: momentary (400 ms). S: short-term (3 s). I: integrated since the last reset, gated. LRA: loudness range. TP: the highest true peak. Click to reset I, LRA and TP, for example before playing the song from the top.";
    case kTipCompact: return paramTip(kCompact);
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
      "CLEAR: transparent; slows its release during sustained limiting so the level does not pump.\n"
      "PUNCHY: shorter attack, half the release; snappy drums.\n"
      "DYNAMIC: recovers faster after deep peaks; keeps movement.\n"
      "BALANCED: soft saturation of the peaks first; warmth, less reduction.\n"
      "AGGRESSIVE: the loudest; fast, with harder saturation. Watch the bass.\n"
      "MODERN: fast after short peaks, slower when limiting continuously.\n"
      "BUS: gentle glue with a long, adaptive release.\n"
      "SAFE: avoids distortion above all; full look-ahead, release never under 40 ms.\n\n"
      "Hover over a style button for its description." },
    { "Look-ahead, attack, release",
      "LOOK-AHEAD (0 to 10 ms) lets Granite see peaks coming and lower the gain smoothly beforehand. It adds the same amount of latency, which your DAW compensates. At 0 ms Granite acts as a clipper.\n\n"
      "ATTACK sets how gradually the gain comes down ahead of a peak, up to the look-ahead time. Longer is smoother; shorter keeps more transient snap.\n\n"
      "RELEASE sets how quickly the gain recovers. Short release is louder but can distort bass and pump; long release is smoother.\n\n"
      "The limit is always exact: whatever you set, the output never goes over the output level." },
    { "Linking & oversampling",
      "CHANNEL LINK has two knobs. TRANSIENT links the fast gain changes between left and right, RELEASE links the recovery. Fully linked keeps the stereo image rock steady; "
      "less linking can be louder, but a loud event on one side will then move the image.\n\n"
      "OVERSAMPLING (2x to 32x) runs the limiter at a higher rate with linear-phase filters. It reduces aliasing and catches most inter-sample peaks; "
      "TRUE PEAK LIMITING catches the rest. 4x or 8x suits most masters; 16x and 32x cost more CPU for the last bit of cleanliness on very dense material." },
    { "True peak & loudness",
      "TRUE PEAK LIMITING keeps the peaks between samples, which appear after D/A conversion or lossy encoding, under the output level. It interpolates the output 4x and adds about 1 ms of latency. Combine it with oversampling for the cleanest result.\n\n"
      "The LOUDNESS readouts measure the output to ITU-R BS.1770 and EBU R128: momentary, short-term, integrated (since you last clicked them), loudness range and true peak. "
      "Choose a LOUDNESS TARGET to see it in the display: R128 -23 LUFS, ATSC A/85 -24, streaming -14 or podcast -16. The integrated value turns green within 1 LU.\n\n"
      "Typical streaming master: TRUE PEAK LIMITING on, output -1 dB, gain until the integrated loudness reaches your goal." },
    { "Sidechain & monitoring",
      "SIDECHAIN EXTERNAL makes the limiter react to its sidechain input. Feed it the full mix and put Granite on a stem: the stem is limited exactly when and as much as the mix is, so stems add up to the limited master.\n\n"
      "UNITY GAIN turns the output down by the GAIN amount, so you can judge the limiting at the original level. AUDITION LIMITING plays only what the limiter removes. Both are for listening only: switch them off before you export. "
      "A badge in the display reminds you while either is on.\n\n"
      "DC FILTER removes any DC offset before limiting.\n\n"
      "COMPACT (in the header) shrinks the window to a meter strip and the controls." },
    { "Dither & metering",
      "DITHER adds a very small, carefully shaped noise when the output is reduced to 16, 20 or 24 bits, so fades and quiet passages stay smooth. Use it only once, as the last "
      "step of a final master, at the bit depth you are exporting. Noise shaping moves that noise to frequencies where it is hardest to hear.\n\n"
      "METERS show input, gain reduction and output. TRUE PEAK METERS makes the output meter estimate inter-sample peaks. METER SCALE switches to the K-System (K-20, K-14, K-12), "
      "DISPLAY RANGE zooms the display and meters to the top 12, 24 or 48 dB, and DISPLAY SPEED shows 5, 10 or 30 seconds of history, or turns the display off." },
    { "A/B, undo & MIDI",
      "A/B holds two complete settings; COPY copies the current one to the other slot. When comparing, match the loudness, because louder always sounds better.\n\n"
      "Every edit is an undo step. Cmd/Ctrl-Z undoes, Cmd/Ctrl-Shift-Z redoes.\n\n"
      "MIDI LEARN: click LEARN, click a control, move a knob on your controller. Right-click to unassign; LEARN again or Esc to finish.\n\n"
      "Double-click a knob to type a value; Cmd/Ctrl-click resets it. Shift-drag is fine adjustment. Esc closes this window." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace granite
