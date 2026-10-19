# Granite 1.0 — User Manual

Granite is a stereo brickwall limiter. It makes audio louder while guaranteeing that nothing goes above the output level you set. It has four limiting styles, adjustable look‑ahead, attack and release, separate channel linking for the transient and release stages, up to 4× linear‑phase oversampling with true‑peak limiting, dither with noise shaping, and a scrolling display of input, output, gain reduction and RMS. Granite runs as VST3, AU, CLAP and VST2 on macOS, Windows and Linux.

**Contents**

1. [Installing](#1-installing)
2. [A quick tour](#2-a-quick-tour)
3. [Getting loud safely](#3-getting-loud-safely)
4. [Styles](#4-styles)
5. [Look‑ahead, attack and release](#5-lookahead-attack-and-release)
6. [Channel linking](#6-channel-linking)
7. [Oversampling and true peaks](#7-oversampling-and-true-peaks)
8. [Dither and noise shaping](#8-dither-and-noise-shaping)
9. [The display and meters](#9-the-display-and-meters)
10. [Knobs](#10-knobs)
11. [A/B, undo and redo](#11-ab-undo-and-redo)
12. [MIDI Learn](#12-midi-learn)
13. [Presets](#13-presets)
14. [Help and hover tips](#14-help-and-hover-tips)
15. [Recipes](#15-recipes)
16. [Keyboard and mouse reference](#16-keyboard-and-mouse-reference)
17. [Parameter reference](#17-parameter-reference)
18. [Troubleshooting](#18-troubleshooting)

---

## 1. Installing

Download the zip for your system from the project's **Releases** page and copy the plugin files:

| System | Copy | To |
|---|---|---|
| macOS | `Granite.vst3`, `Granite.component` (AU), `Granite.clap` | `~/Library/Audio/Plug-Ins/VST3`, `.../Components`, `.../CLAP` |
| Windows | the `Granite.vst3` folder, `Granite.clap` | `C:\Program Files\Common Files\VST3\`, `...\CLAP\` |
| Linux | `Granite.vst3`, `Granite.clap` | `~/.vst3/`, `~/.clap/` |

**macOS:** the builds are signed ad hoc but not notarized, so macOS may block them the first time. Open Terminal and run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Granite*
```

Rescan plugins in your DAW. Granite appears under **Epsilon Audio** as a dynamics/mastering effect.

---

## 2. A quick tour

![Granite](../screenshot.png)

- **Header:**
  - preset browser
  - A/B and COPY
  - UNDO/REDO
  - LEARN (MIDI Learn) and MIDI on/off
  - HELP
  - the current latency, at the far right
- **Level display:** the last ten seconds, scrolling left. It shows:
  - the input in grey
  - the output in blue‑grey
  - the gain reduction in red
  - the output RMS as a yellow line
  - the output level as a dashed line you can drag
- **Meters:** input, gain reduction and output, with peak readouts.
- **Main panel:** STYLE, GAIN, LOOKAHEAD, ATTACK, RELEASE and OUTPUT LEVEL.
- **Advanced panel:** channel link, oversampling, true‑peak meters, dither, noise shaping, meter scale and display range.

---

## 3. Getting loud safely

1. Set **OUTPUT LEVEL** to the highest level you want to allow:
   - **−1 dB** for streaming and lossy formats
   - **−0.3 dB** for CD
   - **−0.1 dB** as a safety limiter inside a mix
2. Set **OVERSAMPLING** to **4×** for final masters, so peaks between samples are limited too.
3. Turn up **GAIN** while you watch the red area in the display and the GR meter:
   - 1–3 dB of gain reduction is usually inaudible.
   - 4–6 dB is loud and still clean on most material.
   - Beyond that, the music starts to flatten.
4. Use **A/B** to compare against a quieter setting, with the loudness matched. Louder always sounds better at first.

The limit is exact. Whatever Gain, style, look‑ahead, attack or release you choose, no output sample goes above the output level.

---

## 4. Styles

| Style | Character | Good for |
|---|---|---|
| **Clear** | The most transparent. When it has to limit for a while, it lengthens its release so the level does not pump. | Mastering, acoustic music, anything delicate |
| **Punchy** | Shorter attack and half the release. Keeps drums snappy and loud. | Electronic, hip‑hop, drum buses |
| **Dynamic** | Recovers faster the deeper it has had to limit, so short peaks leave little trace. | Jazz, acoustic, material with big transients |
| **Balanced** | Rounds the very top of the peaks off with soft saturation before limiting: a touch of warmth and less gain reduction for the same loudness. | Rock, metal, broadcast |

---

## 5. Look‑ahead, attack and release

- **LOOKAHEAD** (0–10 ms):
  - Granite delays the audio by this amount so it can see peaks coming and lower the gain smoothly before they arrive.
  - Your DAW compensates the delay automatically. The current latency is shown at the top right.
  - At 0 ms Granite behaves like a clipper.
- **ATTACK** (0.1–10 ms) sets how gradually the gain comes down ahead of a peak:
  - Longer is smoother and more transparent.
  - Shorter keeps more of the transient's snap.
  - Attack cannot be longer than the look‑ahead. The knob shows "LIMITED BY LOOKAHEAD" when it is.
- **RELEASE** (1 ms – 2 s) sets how quickly the gain recovers:
  - Short release is louder, but can distort bass and make the level pump.
  - Long release is smoother, but holds the level down after loud hits.
  - The Clear and Dynamic styles adapt the release to the music, as described in section 4.

Changing the look‑ahead or the oversampling while audio plays makes Granite fade out and back in over a few milliseconds, because its latency changes.

---

## 6. Channel linking

Granite links the left and right channels in two stages, each with its own knob:

- **TRANSIENT** links the fast gain changes.
- **RELEASE** links the recovery.

At 100%, both channels are turned down together, so a loud event on one side does not move the stereo image. Lower values let each side be limited on its own. That can be louder, but one‑sided peaks will then shift the image.

The defaults (75% transient, 100% release) keep the image steady while allowing a little extra loudness.

---

## 7. Oversampling and true peaks

When audio is converted to analog, or encoded to MP3 or AAC, the waveform between the samples can peak higher than any sample. These are **inter‑sample** or **true** peaks, and they can clip a converter or a codec.

- **OVERSAMPLING** at **2×** or **4×** runs the limiter at a higher sample rate with linear‑phase filters:
  - It limits true peaks as well as sample peaks.
  - It reduces aliasing distortion when limiting hard.
  - The filters add 32 (2×) or 48 (4×) samples of latency.
  - At 4×, Granite keeps true peaks within a few hundredths of a dB of the output level.
- **TRUE PEAK METERS** makes the output meter show the estimated true peak (labelled "OUT TP"). It does not change the sound.

---

## 8. Dither and noise shaping

When a final master is exported at a lower bit depth (for example 16‑bit for CD), the last bits are thrown away. Without dither, quiet passages and fade‑outs turn grainy. **DITHER** adds a tiny amount of noise so that never happens. **NOISE SHAPING** moves that noise to frequencies where the ear is least sensitive.

| Setting | What it does |
|---|---|
| DITHER **Off / 16 / 20 / 24** | Bit depth to dither to. Off passes the full floating‑point output. |
| **Flat** | Plain triangular (TPDF) dither, white noise. |
| **Basic** | Gentle first‑order shaping: less noise in the midrange, more at the top. |
| **Optimized** | Three‑stage shaping to a hearing curve. |
| **Weighted** | Nine‑stage shaping to a hearing curve. The quietest‑sounding option at 44.1 and 48 kHz. |

Use dither once, as the very last step of a final master, at the bit depth you export. Don't dither inside a mix or before more processing.

The output stays on the chosen bit‑depth grid and never goes above the output level.

---

## 9. The display and meters

- **Display**:
  - Drag up or down anywhere to change **Gain**. The mouse wheel does the same, and Shift is for fine steps.
  - Drag the dashed **output level** line to set the output level.
- **DISPLAY RANGE** zooms the display and meters to the top 12, 24 or 48 dB.
- **METER SCALE** switches the scale labels to the K‑System (K‑20, K‑14, K‑12). There, 0 sits at an average level 20, 14 or 12 dB below full scale, which helps you judge loudness consistently.
- **Peak readouts** above the meters hold the highest input, gain reduction and output since you last reset them. Click the meters to reset.

---

## 10. Knobs

- **Drag** the middle of a knob up or down. Hold **Shift** for fine adjustment.
- **Grab the rim** to turn the knob in a circle.
- **Mouse wheel** turns a knob.
- **Double‑click** to type a value, then Enter (or Esc to cancel). Release accepts `1.5s`.
- **Ctrl‑click** (Cmd‑click on macOS) resets to the default.

---

## 11. A/B, undo and redo

- **A / B** hold two complete settings. Click the other letter to switch; the first time, B starts as a copy of A.
- **COPY** copies the current setting into the other slot.
- **UNDO / REDO** step through every edit, including preset loads and A/B switches.
  - Undo: Cmd/Ctrl‑Z
  - Redo: Cmd/Ctrl‑Shift‑Z or Cmd/Ctrl‑Y

---

## 12. MIDI Learn

1. Click **LEARN**.
2. Click a control.
3. Move a knob or fader on your controller.
4. Click **LEARN** again (or press Esc) to finish.

While learning, right‑click a control to unassign it, or use **CLEAR ALL ASSIGNMENTS**. Assignments are saved with your project. The **MIDI** button turns MIDI control off without forgetting them.

---

## 13. Presets

| Group | Presets |
|---|---|
| Mastering | Transparent Master, Loud and Clean, Streaming −1 dBTP, CD 16‑bit Final, Gentle Glue, Vinyl Premaster |
| Genres | EDM, Hip‑Hop, Rock, Pop, Acoustic, Jazz, Metal, Ambient |
| Mix & Bus | Drum Bus, Snare Peaks, Vocal Catch, Bass Leveler, Wide Bus, Pumping |
| Utility | Safety Limiter, Broadcast, Soft Clipper, Dither to 16‑bit, Dither to 24‑bit, K‑14 Metering |

The genre presets are starting points. Set Gain by ear for your material.

- **SAVE CURRENT AS…** stores your own presets, in one of these folders:
  - macOS: `~/Library/Audio/Presets/Epsilon Audio/Granite`
  - Windows: `%APPDATA%\Epsilon Audio\Granite\Presets`
  - Linux: `~/.config/epsilon-audio/granite/presets`
- To delete one of your presets, right‑click it in the browser and click again to confirm.

---

## 14. Help and hover tips

- Rest the pointer on any control, header button, the display, the output level line or the meters. After a moment a tip explains it and shows its value.
- **HELP** opens the in‑plugin pages.
- The switch at the bottom of the help window turns tips on or off.
- **FULL MANUAL (WEB)** opens this manual.

---

## 15. Recipes

- **Streaming master:**
  - Style: Clear
  - Oversampling: 4×
  - Output level: −1 dB
  - Look‑ahead and attack: 5 ms each
  - Release: 150 ms
  - Gain: up to 3–5 dB of gain reduction
- **Loud electronic master:**
  - Style: Punchy
  - Look‑ahead: 1.5 ms
  - Attack: 1 ms
  - Release: 40 ms
  - Oversampling: 4×
  - Output level: −0.5 dB
- **Drum bus punch:** the Drum Bus preset, with links at 50% so each side can hit harder.
- **Catch vocal peaks:** the Vocal Catch preset, with the output level just above the vocal's normal peaks.
- **CD export:**
  - Output level: −0.3 dB
  - Oversampling: 4×
  - Dither: 16 bit, Weighted
  - Export at 16‑bit with no further processing.

---

## 16. Keyboard and mouse reference

| Action | How |
|---|---|
| Change Gain | Drag up/down in the display, or mouse wheel over it |
| Change output level | Drag the dashed line |
| Reset peak readouts | Click the meters |
| Fine adjustment | Shift‑drag, Shift‑wheel |
| Type a value | Double‑click a knob |
| Reset a knob | Ctrl/Cmd‑click |
| Undo / redo | Ctrl/Cmd‑Z / Ctrl/Cmd‑Shift‑Z or Ctrl/Cmd‑Y |
| Step presets | Arrows or mouse wheel on the preset name |
| Close help, finish Learn, cancel typing | Esc |

---

## 17. Parameter reference

New parameters are only ever added at the end, so sessions and automation keep working across versions.

| # | Name | Range | Default |
|---|---|---|---|
| 0 | Gain | 0 – +30 dB | 0 dB |
| 1 | Output Level | −30 – 0 dB | −0.1 dB |
| 2 | Style | Clear / Punchy / Dynamic / Balanced | Clear |
| 3 | Lookahead | 0 – 10 ms | 2 ms |
| 4 | Attack | 0.1 – 10 ms | 2 ms |
| 5 | Release | 1 ms – 2 s | 80 ms |
| 6 | Link Transients | 0 – 100% | 75% |
| 7 | Link Release | 0 – 100% | 100% |
| 8 | Oversampling | Off / 2× / 4× | Off |
| 9 | True Peak Meters | Off / On | On |
| 10 | Dither | Off / 16 / 20 / 24 bit | Off |
| 11 | Noise Shaping | Flat / Basic / Optimized / Weighted | Optimized |
| 12 | Meter Scale (display) | dBFS / K‑20 / K‑14 / K‑12 | dBFS |
| 13 | Display Range (display) | 12 / 24 / 48 dB | 24 dB |
| 14 | MIDI | Off / On | On |
| 15–20 | Outputs for the meters and MIDI Learn | read‑only | — |

---

## 18. Troubleshooting

- **Granite doesn't appear in my DAW.** Check the install folders above and rescan plugins. On macOS, run the `xattr` command from section 1.
- **Audio drops out for a moment when I change look‑ahead or oversampling.** That is intended: the latency changes, so Granite fades out and back in over a few milliseconds. Set these before playback for a final render.
- **The meters show peaks above the output level.** With True Peak on and oversampling off, the meter estimates inter‑sample peaks that a 1× limiter cannot see. Switch oversampling to 2× or 4×.
- **It pumps or distorts.** Lower Gain, lengthen Release, or try the Clear style. On bass‑heavy material, a release below about 30 ms can distort low notes.
- **Hiss on silence.** Dither is on. That is expected at 16 bit; turn dither off unless you are exporting the final master at a lower bit depth.
