/* FUNKY-TRAIN -- an 80s funk machine for VIBE FM.
 * 34 keyboard voices (2-op DX-style FM, analog VA and drawbar organs), eight
 * pad-triggered drum voices, eight driving 16-step grooves and a funk FX rack.
 * Keys/MIDI play the synth; pads 0-7 play drums, pads 8-15 launch grooves.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "vibe.h"
#include "vibe_gfx.h"
#include "vibe_dsp.h"
#include "synth.h"
#include "drums.h"
#include "fx.h"

VIBE_PARALLEL_AUDIO

/* ---- parameters: ids are stable, never rename ------------------------------- */
enum {
  P_PATCH = 0, P_VOLUME, P_OCTAVE, P_TRANSPOSE,
  P_ATTACK, P_RELEASE, P_INDEX, P_DECAY, P_BRIGHT, P_DETUNE,
  P_VEL, P_SPREAD, P_VIB_DEPTH, P_VIB_RATE,
  P_DRUM_GAIN, P_DRUM_TUNE, P_DRUM_DEC, P_DRUM_DRIVE, P_DRUM_TONE,
  P_SEQ_RUN, P_SEQ_PATTERN, P_SEQ_BPM, P_SEQ_SWING, P_SEQ_VEL,
  P_SAT_TYPE, P_DRIVE, P_COMP, P_EQ_LOW, P_EQ_MID, P_EQ_HIGH,
  P_WAH_MODE, P_WAH_SPEED, P_WAH_DEPTH, P_WAH_RES,
  P_WAH_LOW, P_WAH_HIGH, P_WAH_MIX,
  P_CHORUS, P_DIM, P_PHASER, P_ROTARY, P_ROT_SPEED, P_ROT_MIX,
  P_DELAY_MIX, P_DELAY_MS, P_DELAY_FB, P_REVERB_MIX, P_REVERB_ROOM,
  P_GATE, P_MASTER,
  P_COUNT
};

/* The patch table is generated from patches/funk-masters-a.vforge.json.  Only
 * this translation unit needs the FX-override arrays; synth.c sees just the
 * ft_patch voice data. */
#define FT_GENERATE_FX_OVERRIDES
#include "patches.generated.h"

static const char *const ON_OFF[]   = { "OFF", "ON" };
static const char *const SATS[]     = { "TAPE", "TRIODE" };
static const char *const WAYS[]     = { "OFF", "AUTO", "ENV", "TALK" };
static const char *const ROTS[]     = { "STOP", "SLOW", "FAST" };
static const char *const OCTS[]     = { "-2", "-1", "0", "+1", "+2" };

#define PV(id,n,mi,ma,de,u) { id, n, mi, ma, de, u, 0, 0 }
#define PE(id,n,mi,ma,de,u,st,ch) { id, n, mi, ma, de, u, st, ch }

static const vibe_param PARAMS[P_COUNT] = {
  PE("patch",         "Patch",        0, FT_PATCH_COUNT - 1, 0, "", FT_PATCH_COUNT, FT_PATCH_NAMES),
  PV("volume",        "Volume",       0, 1.5f, 0.9f, ""),
  PE("octave",        "Octave",       0, 4,    2,    "", 5, OCTS),
  PV("transpose",     "Transpose",  -12, 12,   0,    "st"),
  PV("attack",        "Attack",       0, 1,    0.08f,"s"),
  PV("release",       "Release",      0, 2,    0.20f,"s"),
  PV("index",         "FM Index",     0, 2,    1.00f, ""),
  PV("decay",         "Decay",        0, 2,    1.00f, ""),
  PV("bright",        "Brightness", 0.3f, 2.5f,1.00f, ""),
  PV("detune",        "Detune",     -25, 25,   0,    "ct"),
  PV("velocity",      "Vel Sens",     0, 1,    0.55f, ""),
  PV("spread",        "Spread",       0, 1,    0.50f, ""),
  PV("vib_depth",     "Vib Depth",    0, 0.1f, 0.0f, ""),
  PV("vib_rate",      "Vib Rate",   0.5f, 12.f, 5.0f, "Hz"),
  PV("drum_gain",     "Drum Level",   0, 1.5f, 0.82f, ""),
  PV("drum_tune",     "Drum Tune",  0.5f, 1.8f,1.00f, ""),
  PV("drum_decay",    "Drum Decay", 0.25f, 2.5f,1.00f, ""),
  PV("drum_drive",    "Drum Drive",   0, 1,    0.22f, ""),
  PV("drum_tone",     "Hat Tone",     0, 1,    0.55f, ""),
  PE("seq_run",       "Groove",       0, 1,    0,    "", 2, ON_OFF),
  PE("seq_pattern",   "Pattern",      0, 7,    0,    "", FT_DRUM_PATTERNS, 0),
  PV("seq_bpm",       "Tempo",       60, 180, 108,  "bpm"),
  PV("seq_swing",     "Swing",        0, 0.7f, 0.18f, ""),
  PV("seq_vel",       "Accent",       0, 1,    0.70f, ""),
  PE("sat_type",      "Sat Type",     0, 1,    0,    "", 2, SATS),
  PV("drive",         "Drive",        0, 1,    0.22f, ""),
  PV("comp",          "Comp",         0, 1,    0.30f, ""),
  PV("eq_low",        "Bass",       -18, 18,   2.0f, "dB"),
  PV("eq_mid",        "Mid",        -18, 18,  -1.5f, "dB"),
  PV("eq_high",       "Treble",     -18, 18,   2.5f, "dB"),
  PE("wah_mode",      "Wah Mode",     0, 3,    0,    "", 4, WAYS),
  PV("wah_speed",     "Wah Speed",  0.1f, 10.f,2.4f, "Hz"),
  PV("wah_depth",     "Wah Depth",    0, 1,    0.85f, ""),
  PV("wah_res",       "Wah Res",    0.7f, 12.f,5.0f, ""),
  PV("wah_low",       "Wah Low",    100, 1500, 300,  "Hz"),
  PV("wah_high",      "Wah High",   800, 7000, 3400, "Hz"),
  PV("wah_mix",       "Wah Mix",      0, 1,    0.70f, ""),
  PV("chorus",        "Chorus",       0, 1,    0.30f, ""),
  PV("dimension",     "Dimension D",  0, 1,    0.25f, ""),
  PV("phaser",        "Phaser",       0, 1,    0.0f, ""),
  PE("rotary",        "Rotary",       0, 1,    0,    "", 2, ON_OFF),
  PE("rot_speed",     "Rotor Speed",  0, 2,    1,    "", 3, ROTS),
  PV("rot_mix",       "Rotor Mix",    0, 1,    0.60f, ""),
  PV("delay_mix",     "Echo",         0, 1,    0.12f, ""),
  PV("delay_ms",      "Echo Time",    0, 1400, 375,  "ms"),
  PV("delay_fb",      "Echo Fb",      0, 0.9f, 0.32f, ""),
  PV("reverb_mix",    "Gated Verb",   0, 1,    0.14f, ""),
  PV("reverb_room",   "Room",         0, 1,    0.45f, ""),
  PV("gate",          "Verb Gate",    0, 1,    0.35f, ""),
  PV("master",        "Master",       0, 1.5f, 0.88f, "")
};
#undef PV
#undef PE

/* Standard-editor mapping: the FX tab collects the whole rack. Delay and
 * gated reverb stay under SPACE; vibrato is an LFO, not an effect. Parameter
 * ids are unchanged - only the cards they appear on move. */
static const char *const GROUPS[P_COUNT] = {
  "SOUND · VOICE","SOUND · VOICE","SOUND · VOICE","SOUND · VOICE",
  "SYNTH · ENGINE","SYNTH · ENGINE","SYNTH · ENGINE","SYNTH · ENGINE",
  "SYNTH · ENGINE","SYNTH · ENGINE","SYNTH · ENGINE","SYNTH · ENGINE",
  "LFO · VIBRATO","LFO · VIBRATO",
  "DRUMS · DRUMS","DRUMS · DRUMS","DRUMS · DRUMS","DRUMS · DRUMS","DRUMS · DRUMS",
  "GROOVE · PATTERN","GROOVE · PATTERN","GROOVE · PATTERN","GROOVE · PATTERN","GROOVE · PATTERN",
  "FX · DYNAMICS","FX · DYNAMICS","FX · DYNAMICS",
  "FX · EQ","FX · EQ","FX · EQ",
  "FX · WAH","FX · WAH","FX · WAH","FX · WAH","FX · WAH","FX · WAH","FX · WAH",
  "FX · MOD FX","FX · MOD FX","FX · MOD FX",
  "FX · ROTARY","FX · ROTARY","FX · ROTARY",
  "SPACE · DELAY","SPACE · DELAY","SPACE · DELAY",
  "SPACE · REVERB","SPACE · REVERB","SPACE · REVERB",
  "SYSTEM · OUTPUT"
};

/* Rack order for the standard editor, so the cards read like the signal flow. */
static const vibe_card CARDS[] = {
  { "VOICE",    6, "34 original funk voices: DX FM, analog VA and drawbar organs." },
  { "ENGINE",   6, "Envelopes, FM index, brightness, velocity and spread." },
  { "VIBRATO",  4, "Pitch LFO depth and rate." },
  { "DRUMS",    6, "808/909-style voices; pads 1-8 trigger, pads 9-16 launch grooves." },
  { "PATTERN",  6, "16-step grooves. PLAY runs one; the DAW transport leads when present." },
  { "DYNAMICS", 4, "Tape / triode saturation and the compressor." },
  { "EQ",       4, "Three-band funk EQ." },
  { "WAH",      6, "Auto, envelope and talk wah: mode, sweep and band limits." },
  { "MOD FX",   4, "Chorus, Dimension D and phaser." },
  { "ROTARY",   6, "Dual-rotor rotary speaker with slow / fast motors." },
  { "DELAY",    6, "Stereo ping-pong echo." },
  { "REVERB",   6, "Gated reverb: room size and gate." },
  { "OUTPUT",   4, "Overall output level." }
};

/* ---- pages ----------------------------------------------------------------- */
enum { UI_SOUND, UI_SYNTH, UI_GROOVE, UI_FX, UI_MOD, UI_SPACE, UI_N };
static const int PAGE_PARAM[UI_N][4] = {
  { P_PATCH, P_OCTAVE, P_TRANSPOSE, P_VOLUME },
  { P_INDEX, P_DECAY, P_BRIGHT, P_ATTACK },
  { P_SEQ_RUN, P_SEQ_PATTERN, P_SEQ_BPM, P_SEQ_SWING },
  { P_SAT_TYPE, P_DRIVE, P_WAH_MODE, P_WAH_MIX },
  { P_CHORUS, P_DIM, P_ROTARY, P_ROT_MIX },
  { P_DELAY_MIX, P_DELAY_MS, P_REVERB_MIX, P_GATE }
};
static const char *const PAGE_NAME[UI_N] = {
  "SOUND", "SYNTH", "GROOVE", "FUNK FX", "MOD", "SPACE"
};
static const char *const F_LABELS[6] = { "PAGE", "PATCH", "STAB", "GROOVE", "HELP", "RUN" };
static const char *const K_LABELS[6] = { "SOUND", "SYNTH", "GROOVE", "FX", "SPACE", "RUN" };

static const char *const HELP[] = {
  "KEYS / MIDI", "play the 80s keyboard",
  "PADS 1-8", "kick, snare, clap, hats, tom, cow, perc",
  "PADS 9-16", "launch the eight driving grooves",
  "PLAY / STOP", "groove on / off",
  "F1 / ENC A", "next page",
  "F2 / ENC B", "next keyboard patch",
  "F3", "audition a funk stab",
  "F4", "next groove",
  "F5 / HOME", "this overview",
  "F6 / K6", "groove run / stop",
  "K1-K5", "SOUND / SYNTH / GROOVE / FX / SPACE",
  "KNOBS 1-4", "edit the four values shown",
  "DATA + ENTER", "FOCUS: walk, press to edit",
  "AUD", "audition a funk stab",
  "PANIC", "all notes off",
  "REC", "panic / all notes off",
  "RETURN", "close overview, back to SOUND",
  "FOCUS", "hold ENTER 0.6 s, then DATA walks"
};

/* ---- state ----------------------------------------------------------------- */
static float sr = 48000.0f;
static int g_params[P_COUNT];               /* atomic, value x1000 */
static float ui_vals[P_COUNT];
static int cur_patch, applying, ui_page, help, draw_tick;
static char status[192];
static int status_len, status_pending;
static int meter_l, meter_r;                /* atomic, 0..1000 */

static int aget(const volatile int *p) { return __atomic_load_n(p, __ATOMIC_ACQUIRE); }
static void aset(int *p, int v) { __atomic_store_n(p, v, __ATOMIC_RELEASE); }
static int scaled(float v) { return (int)(v * 1000.0f + (v >= 0 ? 0.5f : -0.5f)); }
static float q1000(int v) { return (float)v * 0.001f; }

static void set_param(int id, float v);
static void apply_patch(int p);

/* Control-flow -> audio-flow note/drum queue.  1 = note on, 2 = note off,
 * 3 = panic, 4 = drum trigger.  Synth notes are transposed here so a note off
 * always uses the same offset as its note on. */
#define QN 64
static volatile unsigned char q_type[QN], q_note[QN], q_vel[QN];
static volatile unsigned int qw, qr;
static volatile int cur_step = -1;
static volatile int last_note = -1, last_vel, note_hits;
static int pad_flash[16];

static void post(int type, int note, int vel)
{
  unsigned int w, r;
  if (type == 1 || type == 2) {
    if (note < 128) {
      note += ((int)ui_vals[P_OCTAVE] - 2) * 12 + (int)ui_vals[P_TRANSPOSE];
      if (note < 0) note = 0;
      if (note > 127) note = 127;
    }
    last_note = note;
    last_vel = type == 1 ? vel : 0;
    if (type == 1) note_hits++;
  }
  r = qr;
  VIBE_ACQUIRE();
  w = qw;
  if (w - r >= QN) return;
  q_note[w % QN] = (unsigned char)(note & 127);
  q_vel[w % QN] = (unsigned char)(vel & 127);
  q_type[w % QN] = (unsigned char)type;
  VIBE_RELEASE();
  qw = w + 1;
}

static void panic(void)
{
  int i;
  for (i = 0; i < 16; i++) pad_flash[i] = 0;
  post(3, 0, 0);
}

static void audition(void)
{
  post(1, 48, 108);
  post(1, 55, 96);
  post(1, 64, 92);
}

static void store_state(void)
{
  int32_t buf[P_COUNT], i;
  for (i = 0; i < P_COUNT; i++) buf[i] = (int32_t)aget(&g_params[i]);
  vibe_store_write("funky-train-state", buf, (int)sizeof buf);
}

static void load_state(void)
{
  int32_t buf[P_COUNT];
  int i, k;
  float v;
  int n = vibe_store_read("funky-train-state", buf, (int)sizeof buf);
  if (n != (int)sizeof buf) return;
  for (i = 0; i < P_COUNT; i++) {
    v = (float)buf[i] * 0.001f;
    if (v < PARAMS[i].min || v > PARAMS[i].max) return;
  }
  for (i = 0; i < P_COUNT; i++) {
    v = (float)buf[i] * 0.001f;
    if (PARAMS[i].steps > 1) {
      k = (int)(v + 0.5f);
      if (k < 0) k = 0;
      if (k >= PARAMS[i].steps) k = PARAMS[i].steps - 1;
      v = (float)k;
    }
    ui_vals[i] = v;
    aset(&g_params[i], scaled(v));
    vibe_param_set(i, v);
  }
  cur_patch = (int)(ui_vals[P_PATCH] + 0.5f);
  ft_synth_set_patch(cur_patch);
  set_param(P_SEQ_RUN, 0.0f);   /* a groove never auto-starts */
}

static void set_param(int id, float v)
{
  if (id < 0 || id >= P_COUNT) return;
  v = vd_clamp(v, PARAMS[id].min, PARAMS[id].max);
  if (PARAMS[id].steps > 1) {
    int k = (int)(v + 0.5f);
    if (k < 0) k = 0;
    if (k >= PARAMS[id].steps) k = PARAMS[id].steps - 1;
    v = (float)k;
  }
  ui_vals[id] = v;
  aset(&g_params[id], scaled(v));
  vibe_param_set(id, v);
  if (id == P_PATCH && !applying) apply_patch((int)(v + 0.5f));
}

static void reset_fx_defaults(void)
{
  int i;
  for (i = P_SAT_TYPE; i <= P_MASTER; i++) set_param(i, PARAMS[i].def);
}

static void apply_patch(int p)
{
  int i, id;
  if (p < 0) p = 0;
  if (p >= FT_PATCH_COUNT) p = FT_PATCH_COUNT - 1;
  applying = 1;
  cur_patch = p;
  set_param(P_PATCH, (float)p);
  reset_fx_defaults();
  for (i = 0; i < FT_PFX_MAX; i++) {
    id = FT_PFX_ID[i][p];
    if (id >= 0) set_param(id, FT_PFX_VAL[i][p]);
  }
  ft_synth_set_patch(p);
  applying = 0;
}

static void next_patch(int d)
{
  int p = cur_patch + d;
  while (p < 0) p += FT_PATCH_COUNT;
  while (p >= FT_PATCH_COUNT) p -= FT_PATCH_COUNT;
  apply_patch(p);
}

static void nudge(int id, float d)
{
  float v;
  if (id < 0 || id >= P_COUNT) return;
  if (PARAMS[id].steps > 1) {
    v = ui_vals[id] + (d > 0 ? 1.0f : -1.0f);
  } else {
    v = ui_vals[id] + d * (PARAMS[id].max - PARAMS[id].min) / 50.0f;
  }
  set_param(id, v);
}

static int append_txt(char *dst, int at, int cap, const char *s)
{
  while (*s && at < cap - 1) dst[at++] = *s++;
  dst[at] = 0;
  return at;
}

static void build_status(void)
{
  char t[24];
  int n = 0;
  status[0] = 0;
  n = append_txt(status, n, (int)sizeof status, ft_synth_patch_name(cur_patch));
  n = append_txt(status, n, (int)sizeof status, " | voices ");
  n = append_txt(status, n, (int)sizeof status, vg_itoa(ft_synth_voice_count(), t));
  n = append_txt(status, n, (int)sizeof status, " | rotor ");
  n = append_txt(status, n, (int)sizeof status,
                 ft_fx_rotor_hz100() > 150 ? "FAST" : (ft_fx_rotor_hz100() > 20 ? "SLOW" : "STOP"));
  n = append_txt(status, n, (int)sizeof status, " | wah ");
  n = append_txt(status, n, (int)sizeof status, vg_itoa(ft_fx_wah_hz100() / 100, t));
  n = append_txt(status, n, (int)sizeof status, "Hz | note ");
  n = append_txt(status, n, (int)sizeof status,
                 last_note >= 0 ? vg_itoa(last_note, t) : "--");
  n = append_txt(status, n, (int)sizeof status, "/");
  n = append_txt(status, n, (int)sizeof status, vg_itoa(last_vel, t));
  n = append_txt(status, n, (int)sizeof status, " (");
  n = append_txt(status, n, (int)sizeof status, vg_itoa(note_hits, t));
  n = append_txt(status, n, (int)sizeof status, ") | ");
  n = append_txt(status, n, (int)sizeof status,
                 aget(&g_params[P_SEQ_RUN]) ? "groove on" : "groove off");
  status_len = n;
  status_pending = 1;
}

/* ---- editor byte bridge ----------------------------------------------------- */
static void handle_editor(const uint8_t *b, int n)
{
  if (!b || n < 1) return;
  if (b[0] == 0x20) status_pending = 1;
  else if (b[0] == 0x21) store_state();
  else if (b[0] == 0x22) { apply_patch(0); store_state(); }
  else if (b[0] == 0x23) panic();
  else if (b[0] == 0x24) next_patch(1);
  else if (b[0] == 0x25) next_patch(-1);
  else if (b[0] == 0x26 && n >= 2) post(1, b[1], 108);
  else if (b[0] == 0x27 && n >= 2) post(2, b[1], 0);
  else if (b[0] >= 0x30 && b[0] < 0x38) {
    int v = b[0] - 0x30;
    pad_flash[v] = 10;
    post(4, v, 112);
  }
  else if (b[0] == 0x38 && n >= 2) {
    set_param(P_SEQ_PATTERN, (float)(b[1] % FT_DRUM_PATTERNS));
  }
  else if (b[0] == 0x40) set_param(P_SEQ_RUN, ui_vals[P_SEQ_RUN] > 0.5f ? 0.0f : 1.0f);
  else if (b[0] == 0x41 && n >= 2) ft_drum_pattern_restore((int)b[1] % FT_DRUM_PATTERNS);
}

/* ---- controls --------------------------------------------------------------- */
static void handle_button(int id)
{
  switch (id) {
  case VB_F1: ui_page = (ui_page + 1) % UI_N; break;
  case VB_F2: next_patch(1); break;
  case VB_F3: case VB_K6: case VB_PLAY:
    if (id == VB_K6 || id == VB_PLAY)
      set_param(P_SEQ_RUN, ui_vals[P_SEQ_RUN] > 0.5f ? 0.0f : 1.0f);
    else audition();
    break;
  case VB_F4: nudge(P_SEQ_PATTERN, 1); break;
  case VB_F5: case VB_HOME: help = !help; break;
  case VB_F6: set_param(P_SEQ_RUN, ui_vals[P_SEQ_RUN] > 0.5f ? 0.0f : 1.0f); break;
  case VB_STOP: set_param(P_SEQ_RUN, 0.0f); break;
  case VB_REC: panic(); break;
  case VB_K1: ui_page = UI_SOUND; break;
  case VB_K2: ui_page = UI_SYNTH; break;
  case VB_K3: ui_page = UI_GROOVE; break;
  case VB_K4: ui_page = UI_FX; break;
  case VB_K5: ui_page = UI_SPACE; break;
  case VB_RETURN: help = 0; ui_page = UI_SOUND; break;
  default: break;
  }
}

void vibe_on_event(const vibe_event *e)
{
  int fid = -1, r, p;
  if (!e) return;
  r = vf_event(e, &fid);
  if (r == VF_PRESS) {
    if (fid == 900) audition();
    else if (fid == 901) panic();
    return;
  }
  if (r == VF_CHANGED) {
    if (fid >= 0 && fid < P_COUNT) set_param(fid, ui_vals[fid]);
    return;
  }
  if (r == VF_MOVED) return;

  if (e->type == VIBE_EV_NOTE) {
    post(e->down ? 1 : 2, (int)e->id, (int)(e->value * 127.0f + 0.5f));
    return;
  }
  if (e->type == VIBE_EV_MIDI) {
    /* SDK 8 turns MIDI notes into VIBE_EV_NOTE before we see them.  Accept
     * raw notes too, for hosts that still deliver them as VIBE_EV_MIDI. */
    int st = e->midi[0] & 0xF0;
    if (st == 0x90 && e->midi[2] > 0)
      post(1, e->midi[1] & 127, e->midi[2]);
    else if (st == 0x80 || st == 0x90)
      post(2, e->midi[1] & 127, 0);
    return;
  }
  if (e->type == VIBE_EV_KEY) {
    int note = 48 + (int)e->id;
    post(e->down ? 1 : 2, note, e->down ? (int)(e->value * 127.0f + 0.5f) : 0);
    return;
  }
  if (e->type == VIBE_EV_PAD) {
    p = (int)e->id;
    if (p < 0 || p >= 16) return;
    if (e->down) pad_flash[p] = 10;
    if (p < FT_D_COUNT) {
      if (e->down) post(4, p, (int)(e->value * 127.0f + 0.5f));
    } else if (e->down) {
      set_param(P_SEQ_PATTERN, (float)(p - FT_D_COUNT));
      set_param(P_SEQ_RUN, 1.0f);
    }
    return;
  }
  if (e->type == VIBE_EV_PARAM) {
    int i = (int)e->id;
    if (i >= 0 && i < P_COUNT) {
      ui_vals[i] = e->value;
      aset(&g_params[i], scaled(e->value));
      if (i == P_PATCH && !applying) apply_patch((int)(e->value + 0.5f));
    }
    return;
  }
  if (e->type == VIBE_EV_EDITOR_DATA) { handle_editor(vibe_editor_data(), (int)e->value); return; }
  if (e->type == VIBE_EV_SAVE) { store_state(); return; }
  if (e->type == VIBE_EV_BUTTON) { if (e->down) handle_button((int)e->id); return; }
  if (e->type == VIBE_EV_ENCODER) {
    float d = e->value;
    if (d == 0.0f) return;
    if (e->id == VE_A) { ui_page = (ui_page + (d > 0 ? 1 : UI_N - 1)) % UI_N; return; }
    if (e->id == VE_B) { next_patch(d > 0 ? 1 : -1); return; }
    if (e->id >= VE_1 && e->id <= VE_4) { nudge(PAGE_PARAM[ui_page][e->id - VE_1], d); return; }
  }
}

/* ---- audio ------------------------------------------------------------------ */
static float synth_l[VIBE_MAX_BLOCK], synth_r[VIBE_MAX_BLOCK];
static float drum_l[VIBE_MAX_BLOCK], drum_r[VIBE_MAX_BLOCK];
static double seq_pos;
static int seq_last_step;

static void fill_controls(ft_synth_ctrl *sc, ft_drum_ctrl *dc, int *fxa)
{
  sc->index     = q1000(aget(&g_params[P_INDEX]));
  sc->decay     = q1000(aget(&g_params[P_DECAY]));
  sc->bright    = q1000(aget(&g_params[P_BRIGHT]));
  sc->detune    = (float)aget(&g_params[P_DETUNE]) * 0.001f;
  sc->attack    = q1000(aget(&g_params[P_ATTACK]));
  sc->release   = q1000(aget(&g_params[P_RELEASE]));
  sc->vel       = q1000(aget(&g_params[P_VEL]));
  sc->spread    = q1000(aget(&g_params[P_SPREAD]));
  sc->vib_depth = q1000(aget(&g_params[P_VIB_DEPTH]));
  sc->vib_rate  = q1000(aget(&g_params[P_VIB_RATE]));
  sc->volume    = q1000(aget(&g_params[P_VOLUME]));

  dc->gain  = q1000(aget(&g_params[P_DRUM_GAIN]));
  dc->tune  = q1000(aget(&g_params[P_DRUM_TUNE]));
  dc->decay = q1000(aget(&g_params[P_DRUM_DEC]));
  dc->drive = q1000(aget(&g_params[P_DRUM_DRIVE]));
  dc->tone  = q1000(aget(&g_params[P_DRUM_TONE]));

  fxa[FX_SAT_TYPE]      = aget(&g_params[P_SAT_TYPE]);
  fxa[FX_DRIVE]         = aget(&g_params[P_DRIVE]);
  fxa[FX_COMP]          = aget(&g_params[P_COMP]);
  fxa[FX_EQ_LOW]        = aget(&g_params[P_EQ_LOW]);
  fxa[FX_EQ_MID]        = aget(&g_params[P_EQ_MID]);
  fxa[FX_EQ_HIGH]       = aget(&g_params[P_EQ_HIGH]);
  fxa[FX_WAH_MODE]      = aget(&g_params[P_WAH_MODE]);
  fxa[FX_WAH_SPEED]     = aget(&g_params[P_WAH_SPEED]);
  fxa[FX_WAH_DEPTH]     = aget(&g_params[P_WAH_DEPTH]);
  fxa[FX_WAH_RES]       = aget(&g_params[P_WAH_RES]);
  fxa[FX_WAH_LOW]       = aget(&g_params[P_WAH_LOW]);
  fxa[FX_WAH_HIGH]      = aget(&g_params[P_WAH_HIGH]);
  fxa[FX_WAH_MIX]       = aget(&g_params[P_WAH_MIX]);
  fxa[FX_CHORUS]        = aget(&g_params[P_CHORUS]);
  fxa[FX_DIM]           = aget(&g_params[P_DIM]);
  fxa[FX_PHASER]        = aget(&g_params[P_PHASER]);
  fxa[FX_ROTARY]        = aget(&g_params[P_ROTARY]);
  fxa[FX_ROTARY_SPEED]  = aget(&g_params[P_ROT_SPEED]);
  fxa[FX_ROTARY_MIX]    = aget(&g_params[P_ROT_MIX]);
  fxa[FX_DELAY_MIX]     = aget(&g_params[P_DELAY_MIX]);
  fxa[FX_DELAY_MS]      = aget(&g_params[P_DELAY_MS]);
  fxa[FX_DELAY_FB]      = aget(&g_params[P_DELAY_FB]);
  fxa[FX_REVERB_MIX]    = aget(&g_params[P_REVERB_MIX]);
  fxa[FX_REVERB_ROOM]   = aget(&g_params[P_REVERB_ROOM]);
  fxa[FX_GATE]          = aget(&g_params[P_GATE]);
  fxa[FX_MASTER]        = aget(&g_params[P_MASTER]);
}

void vibe_on_audio(const float *in, float *out_l, float *out_r, int frames)
{
  ft_synth_ctrl sc;
  ft_drum_ctrl dc;
  int fxa[FX_COUNT], i, total = frames, n;
  unsigned int t, end;
  int type, nte, vel;
  int pat, run, bpm, host, host_play;
  float swing, accent;
  double steps_per_frame;
  const vibe_transport_info *tp;
  float peak_l = 0.0f, peak_r = 0.0f;

  (void)in;
  if (total <= 0) return;
  n = total > VIBE_MAX_BLOCK ? VIBE_MAX_BLOCK : total;

  t = qr;
  end = qw;
  VIBE_ACQUIRE();
  while (t != end) {
    type = q_type[t % QN];
    nte = q_note[t % QN];
    vel = q_vel[t % QN];
    if (type == 1) ft_synth_note_on(nte, vel, ft_synth_current_patch());
    else if (type == 2) ft_synth_note_off(nte);
    else if (type == 3) { ft_synth_all_off(); ft_drums_reset(); }
    else if (type == 4) ft_drums_trigger(nte, vel);
    t++;
  }
  qr = t;
  VIBE_RELEASE();

  fill_controls(&sc, &dc, fxa);

  /* Groove clock: the DAW leads only while it is actually playing. A stopped
     host must not mute the on-screen pads — the firmware clock keeps them live. */
  tp = vibe_transport();
  host = tp && tp->has && tp->bpm > 0.0f;
  host_play = host && tp->playing;
  run = aget(&g_params[P_SEQ_RUN]) > 500;
  pat = aget(&g_params[P_SEQ_PATTERN]) / 1000;
  bpm = host_play ? (int)tp->bpm : aget(&g_params[P_SEQ_BPM]) / 1000;
  if (bpm < 20) bpm = 108;
  swing = q1000(aget(&g_params[P_SEQ_SWING]));
  accent = q1000(aget(&g_params[P_SEQ_VEL]));
  steps_per_frame = (double)bpm * 4.0 / (60.0 * (double)sr);

  if (!run) {
    seq_pos = 0.0;
    seq_last_step = -1;
    __atomic_store_n(&cur_step, -1, __ATOMIC_RELEASE);
  } else if (host_play) {
    double want = tp->beat * 4.0;          /* quarter notes -> 16th steps */
    if (want < 0.0) want = 0.0;
    if (want - seq_pos > 0.25 || seq_pos - want > 0.25) {
      seq_pos = want;
      seq_last_step = -1;                  /* jumped (start / loop): re-arm */
    }
  }

  for (i = 0; i < n; i++) {
    if (run) {
      int s = (int)seq_pos;
      double ph = seq_pos - (double)s;
      int st = ((s % FT_DRUM_STEPS) + FT_DRUM_STEPS) % FT_DRUM_STEPS;
      double late = (st & 1) ? (double)swing * 0.35 : 0.0;
      if (s != seq_last_step && ph >= late) {
        unsigned char mask = ft_drum_step(pat, st);
        int tr;
        seq_last_step = s;
        __atomic_store_n(&cur_step, st, __ATOMIC_RELEASE);
        for (tr = 0; tr < FT_D_COUNT; tr++) {
          if (mask & (1u << tr))
            ft_drums_trigger(tr, (int)(105.0f * accent + (st % 4 == 0 ? 22.0f : 0.0f)));
        }
      }
      seq_pos += steps_per_frame;
    }
  }

  ft_synth_render(synth_l, synth_r, n, &sc);
  ft_drums_render(drum_l, drum_r, n, &dc);
  for (i = 0; i < n; i++) {
    out_l[i] = synth_l[i] + drum_l[i];
    out_r[i] = synth_r[i] + drum_r[i];
  }
  ft_fx_process(out_l, out_r, n, fxa);

  for (i = 0; i < n; i++) {
    float al = vd_absf(out_l[i]), ar = vd_absf(out_r[i]);
    peak_l = al > peak_l ? al : peak_l;
    peak_r = ar > peak_r ? ar : peak_r;
  }
  __atomic_store_n(&meter_l, scaled(peak_l), __ATOMIC_RELEASE);
  __atomic_store_n(&meter_r, scaled(peak_r), __ATOMIC_RELEASE);

  for (i = n; i < total; i++) { out_l[i] = 0.0f; out_r[i] = 0.0f; }
}

/* ---- display ---------------------------------------------------------------- */
#define C_BG      0x100C14
#define C_PANEL   0x1D1428
#define C_PANEL2  0x2A1B39
#define C_ACC     0xFF9F1C
#define C_GOLD    0xF2C14E
#define C_CREAM   0xFFF0D0
#define C_AVO     0x8FA44B
#define C_RED     0xE04A4A
#define C_BLUE    0x46B6FF
#define C_VIOLET  0xB987FF
#define C_MUTED   0xA08DB5

static void draw_train(float x, float y, float t)
{
  /* one small 80s motif, drawn from primitives: a train in a neon sunset */
  vg_circle(x, y - 3, 15.0f, C_GOLD);
  vg_rect(x - 23, y - 4, 46, 3, C_BG);
  vg_rect(x - 23, y + 3, 46, 3, C_BG);
  vg_round(x - 19, y + 9, 38, 12, 3, C_VIOLET);
  vg_rect(x - 14, y + 12, 9, 5, C_CREAM);
  vg_rect(x - 1, y + 12, 9, 5, C_CREAM);
  vg_circle(x - 12, y + 21, 2.5f, C_ACC);
  vg_circle(x + 12, y + 21, 2.5f, C_ACC);
  vg_line(x - 24, y + 25, x + 24, y + 25, 1.0f, C_BLUE);
  vg_line(x - 24 + (int)(t * 8.0f) % 8, y + 27, x - 18 + (int)(t * 8.0f) % 8,
          y + 27, 1.0f, C_ACC);
}

static int param_decimals(int id)
{
  float span = PARAMS[id].max - PARAMS[id].min;
  if (span <= 3.0f) return 2;
  if (span <= 100.0f) return 1;
  return 0;
}

void vibe_on_draw(uint16_t *fb)
{
  int i, id, st, pat;
  char val[24], val2[24];
  float t, ml, mr;
  vg_begin(fb);

  if (help) {
    vf_begin();
    vf_enable(0);
    vf_end(C_ACC);
    vg_controls("FUNKY-TRAIN CONTROLS", HELP, (int)(sizeof HELP / sizeof HELP[0]) / 2);
    draw_tick++;
    if (status_pending && vibe_editor_post((const uint8_t *)status, status_len) >= 0)
      status_pending = 0;
    return;
  }

  vf_enable(1);

  vg_clear(C_BG);
  vg_rect(0, 0, 480, 44, C_PANEL);
  vg_rect(0, 43, 480, 2, C_ACC);
  draw_train(452, 22, (float)draw_tick * 0.05f);
  vg_text("FUNKY-TRAIN", 14, 7, VG_BIG, C_GOLD, VG_LEFT);
  vg_text(ft_synth_patch_name(cur_patch), 16, 29, VG_SMALL, C_CREAM, VG_LEFT);
  vg_text("NOTE", 180, 29, VG_SMALL, C_MUTED, VG_LEFT);
  vg_text(last_note >= 0 ? vg_itoa(last_note, val2) : "--", 220, 29,
          VG_SMALL, last_note >= 0 ? C_GOLD : C_MUTED, VG_LEFT);
  vg_text(PAGE_NAME[ui_page], 400, 13, VG_BOLD, C_ACC, VG_RIGHT);

  vf_begin();
  for (i = 0; i < 4; i++) {
    int y = 58 + i * 47;
    float step;
    id = PAGE_PARAM[ui_page][i];
    vg_round(10, y, 460, 43, 5, i & 1 ? C_PANEL : C_PANEL2);
    vg_text(PARAMS[id].name, 22, y + 3, VG_SMALL, C_MUTED, VG_LEFT);
    if (PARAMS[id].steps > 1) {
      int k = (int)(ui_vals[id] + 0.5f);
      if (k < 0) k = 0;
      if (k >= PARAMS[id].steps) k = PARAMS[id].steps - 1;
      vg_text(PARAMS[id].choices[k], 458, y + 3, VG_MED, C_CREAM, VG_RIGHT);
    } else {
      vg_text(vg_ftoa(ui_vals[id], param_decimals(id), val), 458, y + 3, VG_BIG, C_CREAM, VG_RIGHT);
    }
    t = (ui_vals[id] - PARAMS[id].min) / (PARAMS[id].max - PARAMS[id].min);
    t = vd_clamp(t, 0.0f, 1.0f);
    vg_rect(22, y + 32, 420, 5, C_BG);
    vg_rect(22, y + 32, (int)(420.0f * t), 5, C_ACC);
    step = PARAMS[id].steps > 1 ? 1.0f : (PARAMS[id].max - PARAMS[id].min) / 100.0f;
    vf_value(id, 10, y, 460, 43, &ui_vals[id], PARAMS[id].min, PARAMS[id].max, step);
  }

  /* 16-step groove grid */
  st = aget(&cur_step);
  pat = (int)(ui_vals[P_SEQ_PATTERN] + 0.5f);
  for (i = 0; i < FT_DRUM_STEPS; i++) {
    int x = 14 + i * 28;
    int hit = ft_drum_step(pat, i) != 0;
    vg_round(x, 250, 24, 10, 2, i == st ? C_ACC : (hit ? C_VIOLET : C_PANEL2));
  }
  vg_text(ft_drum_pattern_name(pat), 470, 249, VG_SMALL, C_MUTED, VG_RIGHT);

  /* meters, voices and focus buttons */
  ml = q1000(aget(&meter_l)); mr = q1000(aget(&meter_r));
  ml = vd_clamp(ml, 0.0f, 1.0f); mr = vd_clamp(mr, 0.0f, 1.0f);
  vg_text("L", 14, 267, VG_SMALL, C_MUTED, VG_LEFT);
  vg_rect(28, 269, 130, 6, C_PANEL2);
  vg_rect(28, 269, (int)(130.0f * ml), 6, ml > 0.85f ? C_RED : C_AVO);
  vg_text("R", 170, 267, VG_SMALL, C_MUTED, VG_LEFT);
  vg_rect(184, 269, 130, 6, C_PANEL2);
  vg_rect(184, 269, (int)(130.0f * mr), 6, mr > 0.85f ? C_RED : C_AVO);
  vg_text("VOICES", 330, 267, VG_SMALL, C_MUTED, VG_LEFT);
  vg_text(vg_itoa(ft_synth_voice_count(), val), 402, 266, VG_BOLD, C_GOLD, VG_RIGHT);
  vg_round(414, 261, 54, 14, 3, C_PANEL2);
  vg_text("AUD", 441, 262, VG_SMALL, C_CREAM, VG_CENTER);
  vg_round(414, 275, 54, 14, 3, C_PANEL2);
  vg_text("PANIC", 441, 276, VG_SMALL, C_CREAM, VG_CENTER);
  vf_button(900, 414, 261, 54, 14);
  vf_button(901, 414, 275, 54, 14);

  vg_rect(0, 289, 480, 2, C_ACC);
  vg_rect(0, 291, 480, 29, C_PANEL);
  for (i = 0; i < 6; i++) {
    vg_text(F_LABELS[i], 8 + i * 79, 292, VG_SMALL,
            i == 1 ? C_GOLD : C_MUTED, VG_LEFT);
    vf_key(VB_F1 + i, 4 + i * 79, 290, 76, 16);
  }
  for (i = 0; i < 6; i++) {
    vg_text(K_LABELS[i], 8 + i * 79, 306, VG_SMALL, C_AVO, VG_LEFT);
    vf_key(VB_K1 + i, 4 + i * 79, 304, 76, 16);
  }
  vf_end(C_ACC);

  /* pad LEDs: 1-8 are drums, 9-16 are groove launchers */
  pat = (int)(ui_vals[P_SEQ_PATTERN] + 0.5f);
  for (i = 0; i < 16; i++) {
    uint32_t c = C_PANEL2;
    float lvl = 0.0f;
    if (pad_flash[i] > 0) {
      c = i < 8 ? (i & 1 ? C_ACC : C_VIOLET) : C_AVO;
      lvl = (float)pad_flash[i] / 10.0f;
      pad_flash[i]--;
    } else if (i >= 8 && i - 8 == pat) {
      c = C_AVO; lvl = 0.45f;
    } else if (i < 8 && st >= 0 && (ft_drum_step(pat, st) & (1u << i))) {
      c = C_ACC; lvl = 0.25f;
    }
    vibe_led_pad(i, c, lvl);
  }
  vibe_led_button(VB_PLAY, aget(&g_params[P_SEQ_RUN]) ? C_AVO : C_PANEL2, 0.7f);

  for (i = 0; i < 6; i++) vibe_label(VB_F1 + i, F_LABELS[i]);
  for (i = 0; i < 6; i++) vibe_label(VB_K1 + i, K_LABELS[i]);
  vibe_link_page(PAGE_NAME[ui_page]);
  for (i = 0; i < 4; i++) {
    int pid = PAGE_PARAM[ui_page][i];
    float v = (ui_vals[pid] - PARAMS[pid].min) /
              (PARAMS[pid].max - PARAMS[pid].min);
    vg_ftoa(vd_clamp(v, 0.0f, 1.0f), 2, val);
    vibe_link_encoder(VE_1 + i, PARAMS[pid].name, val);
  }

  draw_tick++;
  if ((draw_tick % 8) == 0) build_status();
  if (status_pending && vibe_editor_post((const uint8_t *)status, status_len) >= 0)
    status_pending = 0;
}

/* ---- init ------------------------------------------------------------------- */
void vibe_on_init(float sample_rate)
{
  int i;
  sr = sample_rate > 1000.0f ? sample_rate : 48000.0f;
  ft_synth_init(sr);
  ft_drums_init(sr);
  ft_fx_init(sr);

  for (i = 0; i < P_COUNT; i++) {
    ui_vals[i] = PARAMS[i].def;
    aset(&g_params[i], scaled(PARAMS[i].def));
  }
  vibe_params(PARAMS, P_COUNT);
  vibe_param_groups(GROUPS);
  vibe_editor_cards(CARDS, (int)(sizeof CARDS / sizeof CARDS[0]));
  vibe_editor_accent(C_ACC);
  vibe_editor_tab_color("SOUND", C_ACC);
  vibe_editor_tab_color("SYNTH", C_GOLD);
  vibe_editor_tab_color("LFO", 0xFF6EC7);
  vibe_editor_tab_color("DRUMS", C_VIOLET);
  vibe_editor_tab_color("GROOVE", C_AVO);
  vibe_editor_tab_color("FX", C_BLUE);
  vibe_editor_tab_color("SPACE", C_RED);
  vibe_editor_tab_color("SYSTEM", C_CREAM);

  vibe_editor_control(P_PATCH, VIBE_CTL_MENU);
  vibe_editor_control(P_OCTAVE, VIBE_CTL_MENU);
  vibe_editor_control(P_SEQ_PATTERN, VIBE_CTL_MENU);
  vibe_editor_control(P_SEQ_RUN, VIBE_CTL_TOGGLE);
  vibe_editor_control(P_SAT_TYPE, VIBE_CTL_MENU);
  vibe_editor_control(P_WAH_MODE, VIBE_CTL_MENU);
  vibe_editor_control(P_ROTARY, VIBE_CTL_TOGGLE);
  vibe_editor_control(P_ROT_SPEED, VIBE_CTL_MENU);
  vibe_editor_control(P_TRANSPOSE, VIBE_CTL_BIPOLAR);
  vibe_editor_control(P_DETUNE, VIBE_CTL_BIPOLAR);
  vibe_editor_control(P_EQ_LOW, VIBE_CTL_BIPOLAR);
  vibe_editor_control(P_EQ_MID, VIBE_CTL_BIPOLAR);
  vibe_editor_control(P_EQ_HIGH, VIBE_CTL_BIPOLAR);

  vibe_param_help(P_PATCH, "34 original voices from the .vforge Bank A. Pads 1-8 play drums, pads 9-16 launch grooves.");
  vibe_param_help(P_INDEX, "DX-style 2-operator modulation index: more means brighter, harder tines.");
  vibe_param_help(P_WAH_MODE, "Auto, envelope or talk wah on the synth/drum mix.");
  vibe_param_help(P_ROTARY, "Dual-rotor Leslie-style speaker with independent horn and drum motors.");
  vibe_param_help(P_SEQ_RUN, "PLAY runs the selected groove. With a DAW transport, the host clock leads.");
  vibe_param_help(P_SEQ_SWING, "Delays the off-beat sixteenths for a laid-back funk shuffle.");

  apply_patch(0);
  load_state();

  vibe_notes(VIBE_NOTES);
  vibe_keys_octave(2);
  vf_mode(VF_ON_HOLD);
  vf_clicks(1);
  vf_set_focus(P_VOLUME);

  qw = qr = 0;
  seq_pos = 0.0;
  seq_last_step = -1;
  __atomic_store_n(&cur_step, -1, __ATOMIC_RELEASE);
  __atomic_store_n(&meter_l, 0, __ATOMIC_RELEASE);
  __atomic_store_n(&meter_r, 0, __ATOMIC_RELEASE);
  ui_page = UI_SOUND;
  help = 0;
  draw_tick = 0;
  build_status();
}
