# FUNKY-TRAIN

An 80s funk machine for [VIBE FM](https://github.com/Vibe-FM-SDK/vibe-fm-sdk): 34 original keyboard voices, eight synthesized drum voices, eight driving 16-step grooves and a funk FX rack. Keys and MIDI play the synth; pads 1–8 trigger drums and pads 9–16 launch grooves. It plays immediately — no setup, no samples, no impulse responses.

## Sound

- **34 keyboard voices** in Bank A: DX-style 2-op FM bass, EPs, clavs and bells; OB/Juno/Moog-style analog VA brass, poly pads and leads; two additive drawbar organs. Names and high-level recipes live in `patches/funk-masters-a.vforge.json`.
- **Drum machine**: synthesized kick, snare, clap, closed/open hats, tom, cowbell and percussion. The eight grooves are original 16-step patterns: `DAH FUNK`, `BOOGIE`, `ELECTRO`, `GAP BAND`, `CAMEO`, `SLOW JAM`, `THE BREAK`, `FUNK DUB`.
- **Funk FX rack**: tape/triode saturation → compressor → 3-band EQ → auto/envelope/talk wah → stereo chorus → Dimension D → phaser → dual-rotor rotary → ping-pong echo → gated reverb → master.
- **Patch-aware FX**: each `.vforge` voice carries sparse FX overrides, applied on top of safe defaults when a patch is selected.
- **Groove clock**: follows the DAW transport when present, otherwise runs from the firmware's own BPM knob.

## Controls

The base VIBE layout is used throughout. Every value on the display is reachable with **DATA + ENTER** (FOCUS); `HOME` tapped shows the page overview.

| Control | Action |
| --- | --- |
| Keys / MIDI notes | Play the 80s keyboard |
| Pads 1–8 | Trigger kick, snare, clap, hats, tom, cowbell, percussion |
| Pads 9–16 | Launch the eight grooves |
| Encoder A / F1 | Next display page |
| Encoder B / F2 | Next keyboard patch |
| Knobs 1–4 | Edit the four values shown above F1–F6 |
| F3 | Audition a funk stab |
| F4 | Next groove |
| F5 / HOME | Controls overview |
| F6 / PLAY / K6 | Groove run / stop |
| STOP | Groove stop |
| K1–K6 | SOUND · SYNTH · GROOVE · FX · SPACE · RUN |
| REC | Panic / all notes off |
| RETURN | Back to SOUND |
| FOCUS | Hold ENTER 0.6 s, then DATA walks the displayed controls |

Pages are `SOUND`, `SYNTH`, `GROOVE`, `FUNK FX`, `MOD`, `SPACE`. The optional custom Editor page adds a keyboard, drum pads, groove launchers, the 34-voice patch list and the device display.

## Build

Requirements: Zig 0.13.0, Node.js, Python 3, `sh`.

```sh
# If the SDK is missing, fetch the pinned commit:
sh scripts/fetch-sdk.sh

# Build:
cd sdk
sh tools/build.sh ../firmware/funky-train
# -> build/funky-train.wasm, build/funky-train.zip, build/funky-train.editor/, build/funky-train.png
```

On Windows, `python3` can be a Microsoft Store alias. `scripts/build.ps1` sets up a local `python3.exe` shim and calls the same build script:

```powershell
pwsh -File scripts/build.ps1
```

Drop `sdk/build/funky-train.zip` onto the VIBE FM VST window, or load the firmware in the browser emulator.

## Validate

```sh
node sdk/tools/vibe-check.mjs sdk/build/funky-train.wasm \
  --manifest firmware/funky-train/firmware.json --seconds 5

mkdir -p sdk/build/funky-train-test
node sdk/tools/wasmtest.mjs sdk/build/funky-train.wasm \
  firmware/funky-train/wasmtest.json sdk/build/funky-train-test

mkdir -p sdk/build/funky-train-midi-test
node sdk/tools/wasmtest.mjs sdk/build/funky-train.wasm \
  firmware/funky-train/wasmtest-midi.json sdk/build/funky-train-midi-test

mkdir -p sdk/build/funky-train-length-test
node sdk/tools/wasmtest.mjs sdk/build/funky-train.wasm \
  firmware/funky-train/wasmtest-length.json sdk/build/funky-train-length-test

mkdir -p sdk/build/funky-train-groove-test
node sdk/tools/wasmtest.mjs sdk/build/funky-train.wasm \
  firmware/funky-train/wasmtest-grooves.json sdk/build/funky-train-groove-test
```

`vibe-check` verifies format, exports, parallel audio, FOCUS, HOME, storage, parameter/editor metadata, NaN safety and real-time limits. `wasmtest` renders audio and display snapshots for the ten scenes in `firmware/funky-train/wasmtest.json`. The small `wasmtest-midi.json` regression drives a raw MIDI note-on/note-off (status `0x90`/`0x80`) and must produce non-zero audio. `wasmtest-length.json` holds five VA/organ notes for two seconds each and checks that the late-sustain level stays audible. `wasmtest-grooves.json` presses pads 9–16 with a DAW transport present but stopped; every pad must produce audio.

To regenerate the menu icon:

```sh
python tools/make_icon.py       # writes firmware/funky-train/icon.png (960 x 540)
```

## .vforge patch workflow

`patches/funk-masters-a.vforge.json` is the canonical source for Bank A. Generate the firmware table and the readable documents with:

```sh
python tools/vforge.py build-c   patches/funk-masters-a.vforge.json firmware/funky-train/patches.generated.h
python tools/vforge.py spec      patches/funk-masters-a.vforge.json BANK-A.md
python tools/vforge.py dx7-check patches/funk-masters-a.vforge.json DX7-MISSING.md
```

`BANK-A.md` is the readable patch spec. `DX7-MISSING.md` records why no SysEx is shipped: the high-level specs do not include all six operators' envelopes, levels, keyboard scaling, detune, velocity sensitivity, pitch EG or LFO data. Real DX7 SysEx generation is intentionally deferred — **0/34 voices are fully specified**. To enable it, add complete `dx7` operator data to the `.vforge` source and mark each voice `dx7.complete = true`; the encoder can then be added without changing the source format.

## Layout

```text
firmware/funky-train/
  firmware.json          VIBE FM manifest (id: funky-train, protocol 7)
  main.c                 parameters, patches, events, audio flow, display, editor bridge
  synth.c / synth.h      FM / VA / drawbar voice engine
  drums.c / drums.h      synthesized drums and eight 16-step grooves
  fx.c / fx.h            funk FX rack
  patches.generated.h    generated from the .vforge source (do not edit)
  editor/index.html      optional custom Editor page (window.vibe only)
  icon.png               960 x 540 menu tile
  wasmtest.json          scripted audio/display test
  wasmtest-midi.json     raw MIDI note-on/off regression
  wasmtest-length.json   held-note sustain regression (VA + organ)
  wasmtest-grooves.json  pads 9-16 with a stopped DAW transport
patches/
  funk-masters-a.vforge.json  canonical Bank A source
tools/
  vforge.py              .vforge -> C, spec sheet and DX7 completeness report
  make_icon.py           stdlib-only icon generator
scripts/
  build.ps1              Windows build wrapper
  fetch-sdk.sh           pins the VIBE FM SDK commit
sdk/                     VIBE FM SDK (Apache-2.0), pinned at 2ac234088ee5e2da4503f380f3a167d253cc75d4
BANK-A.md                generated readable patch specs
DX7-MISSING.md           generated DX7 completeness report
```

The audio flow owns the voices, drums, sequencer and FX state. The control flow publishes parameters as atomic scaled integers and hands notes over a small SPSC queue. The custom Editor page is a second face only — the firmware remains fully playable without it, on STANDARD cards and on the hardware.

"VIBE FM" is a trademark of its owners; FUNKY-TRAIN is an independent firmware built with the VIBE FM SDK.
