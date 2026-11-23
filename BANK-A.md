# BANK A - FUNK MASTERS

Cameo / Gap Band / D-Train / SOS Band inspired voices. High-level design specs, not raw SysEx.

These are design specifications for a common `.vforge` source. They are not raw DX7 SysEx.

| # | Voice | Type | Algorithm | Ratios | Character | Engine (Vibe FM) |
|---|-------|------|-----------|--------|-----------|------------------|
| 01 | WRDUPBAS | FM Bass | 5 | 1, 1, 0.5, 2 | rubbery, punchy | fm |
| 02 | STRNGBAS | Slap Bass | 4 | 0.5, 1, 2, 4 | punchy | fm |
| 03 | GAPBASS | Synth Bass | 3 | 1, 1, 1.01, 2 | mono, portamento 10 | fm |
| 04 | ZAPPBASS | Talk Bass | 8 | 1, 2, 3, 4 | filter mod macro | fm |
| 05 | CANDYBEL | Bell | 20 | 1, 3, 6, 8, 12 | long release | fm |
| 06 | CRYSTBEL | Glass Bell | 21 | 1, 2.71, 5, 10 | bright | fm |
| 07 | FMBELLPD | Bell Pad | 16 | - | slow attack, long release | fm |
| 08 | MARIMBA1 | FM Marimba | 13 | - | percussive | fm |
| 09 | STRNGBRS | Funk Brass | 7 | - | fast attack | va |
| 10 | OBBRASS1 | OB Emulation | 6 | - | bright, wide | va |
| 11 | OBBRASS2 | Power Brass | 6 | - | detuned | va |
| 12 | HITBRASS | Orchestra Hit | 10 | - | aggressive | va |
| 13 | CANDYEP | EP | 5 | - | warm | fm |
| 14 | DXFUNKEP | Funk EP | 5 | - | velocity sensitive | fm |
| 15 | SLOWEP | Ballad EP | 4 | - | soft | fm |
| 16 | BRITEEP | Bright EP | 5 | - | attack bell | fm |
| 17 | FUNKCLAV | Clav | 8 | - | short decay, high velocity sensitivity | fm |
| 18 | WAHCLAV | Auto Wah Clav | 8 | - | envelope mod | fm |
| 19 | DIRTYCLV | Driven Clav | 8 | - | high feedback | fm |
| 20 | SNAPCLAV | Pop Clav | 8 | - | tight envelopes | fm |
| 21 | JUNOCOMP | Poly Comp | 15 | - | slow chorus | va |
| 22 | SINGLELF | Synth Chords | 14 | - | wide stereo | va |
| 23 | STYLEPOLY | Funk Poly | 15 | - | Funk Poly | va |
| 24 | SHALAPAD | Warm Pad | 16 | - | slow attack | va |
| 25 | CS80STR | String Ensemble | 16 | - | aftertouch vibrato | va |
| 26 | ATLSTRNG | Soul Strings | 17 | - | Soul Strings | va |
| 27 | FUNKPAD1 | Analog Pad | 16 | - | Analog Pad | va |
| 28 | MIDNPAD | Midnight Star Pad | 17 | - | bright | va |
| 29 | MINIFUNK | Lead | 3 | 0.5, 1, 1.01, 2 | mono, portamento | va |
| 30 | TALKLEAD | Vocal Lead | 8 | - | mod wheel to timbre | fm |
| 31 | ZAPPLEAD | Funk Lead | 2 | - | heavy feedback | fm |
| 32 | CAMEOSTK | Mega Layer | - | - | bass + bell + poly + lead layers, performance patch | va |
| 33 | BT ROTARY | Bonus - Rotary Organ | - | - | from the original 80s pack shortlist | organ |
| 34 | BT FUNK B3 | Bonus - Funk B3 | - | - | from the original 80s pack shortlist | organ |

## Detailed specifications

### 01 WRDUPBAS

- Type: FM Bass
- Algorithm: 5
- Feedback: 4
- Ratios: 1, 1, 0.5, 2
- Character: rubbery, punchy
- Operator levels: 99, 88, 70, 35
- LFO: triangle, speed 20, delay 0
- Mono: on, portamento 0
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 02 STRNGBAS

- Type: Slap Bass
- Algorithm: 4
- Feedback: 5
- Ratios: 0.5, 1, 2, 4
- Character: punchy
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 03 GAPBASS

- Type: Synth Bass
- Algorithm: 3
- Feedback: -
- Ratios: 1, 1, 1.01, 2
- Character: mono, portamento 10
- Mono: on, portamento 10
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 04 ZAPPBASS

- Type: Talk Bass
- Algorithm: 8
- Feedback: -
- Ratios: 1, 2, 3, 4
- Character: filter mod macro
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 05 CANDYBEL

- Type: Bell
- Algorithm: 20
- Feedback: 1
- Ratios: 1, 3, 6, 8, 12
- Character: long release
- Operator levels: 99, 75, 65, 40, 20
- LFO: sine, speed 18, delay 0
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 06 CRYSTBEL

- Type: Glass Bell
- Algorithm: 21
- Feedback: -
- Ratios: 1, 2.71, 5, 10
- Character: bright
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 07 FMBELLPD

- Type: Bell Pad
- Algorithm: 16
- Feedback: -
- Ratios: -
- Character: slow attack, long release
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 08 MARIMBA1

- Type: FM Marimba
- Algorithm: 13
- Feedback: -
- Ratios: -
- Character: percussive
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 09 STRNGBRS

- Type: Funk Brass
- Algorithm: 7
- Feedback: -
- Ratios: -
- Character: fast attack
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 10 OBBRASS1

- Type: OB Emulation
- Algorithm: 6
- Feedback: -
- Ratios: -
- Character: bright, wide
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 11 OBBRASS2

- Type: Power Brass
- Algorithm: 6
- Feedback: -
- Ratios: -
- Character: detuned
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 12 HITBRASS

- Type: Orchestra Hit
- Algorithm: 10
- Feedback: -
- Ratios: -
- Character: aggressive
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 13 CANDYEP

- Type: EP
- Algorithm: 5
- Feedback: -
- Ratios: -
- Character: warm
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 14 DXFUNKEP

- Type: Funk EP
- Algorithm: 5
- Feedback: -
- Ratios: -
- Character: velocity sensitive
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 15 SLOWEP

- Type: Ballad EP
- Algorithm: 4
- Feedback: -
- Ratios: -
- Character: soft
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 16 BRITEEP

- Type: Bright EP
- Algorithm: 5
- Feedback: -
- Ratios: -
- Character: attack bell
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 17 FUNKCLAV

- Type: Clav
- Algorithm: 8
- Feedback: 5
- Ratios: -
- Character: short decay, high velocity sensitivity
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 18 WAHCLAV

- Type: Auto Wah Clav
- Algorithm: 8
- Feedback: -
- Ratios: -
- Character: envelope mod
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 19 DIRTYCLV

- Type: Driven Clav
- Algorithm: 8
- Feedback: -
- Ratios: -
- Character: high feedback
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 20 SNAPCLAV

- Type: Pop Clav
- Algorithm: 8
- Feedback: -
- Ratios: -
- Character: tight envelopes
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 21 JUNOCOMP

- Type: Poly Comp
- Algorithm: 15
- Feedback: -
- Ratios: -
- Character: slow chorus
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 22 SINGLELF

- Type: Synth Chords
- Algorithm: 14
- Feedback: -
- Ratios: -
- Character: wide stereo
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 23 STYLEPOLY

- Type: Funk Poly
- Algorithm: 15
- Feedback: -
- Ratios: -
- Character: -
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 24 SHALAPAD

- Type: Warm Pad
- Algorithm: 16
- Feedback: -
- Ratios: -
- Character: slow attack
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 25 CS80STR

- Type: String Ensemble
- Algorithm: 16
- Feedback: -
- Ratios: -
- Character: aftertouch vibrato
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 26 ATLSTRNG

- Type: Soul Strings
- Algorithm: 17
- Feedback: -
- Ratios: -
- Character: -
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 27 FUNKPAD1

- Type: Analog Pad
- Algorithm: 16
- Feedback: -
- Ratios: -
- Character: -
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 28 MIDNPAD

- Type: Midnight Star Pad
- Algorithm: 17
- Feedback: -
- Ratios: -
- Character: bright
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 29 MINIFUNK

- Type: Lead
- Algorithm: 3
- Feedback: 7
- Ratios: 0.5, 1, 1.01, 2
- Character: mono, portamento
- Mono: on, portamento 15
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 30 TALKLEAD

- Type: Vocal Lead
- Algorithm: 8
- Feedback: -
- Ratios: -
- Character: mod wheel to timbre
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 31 ZAPPLEAD

- Type: Funk Lead
- Algorithm: 2
- Feedback: -
- Ratios: -
- Character: heavy feedback
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 32 CAMEOSTK

- Type: Mega Layer
- Algorithm: -
- Feedback: -
- Ratios: -
- Character: bass + bell + poly + lead layers, performance patch
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 33 BT ROTARY

- Type: Bonus - Rotary Organ
- Algorithm: -
- Feedback: -
- Ratios: -
- Character: from the original 80s pack shortlist
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

### 34 BT FUNK B3

- Type: Bonus - Funk B3
- Algorithm: -
- Feedback: -
- Ratios: -
- Character: from the original 80s pack shortlist
- DX7 SysEx: not available from this high-level spec; requires all 6 operator envelopes, levels, scaling and detune.

