/* FUNKY-TRAIN -- 80s funk keyboard engine.
 * FM (DX slap / EP / clav / bell), VA (OB brass, Juno poly, Moog lead) and
 * additive drawbars (rotary organ). No libc, no host calls, all audio-flow.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "synth.h"
#include "patches.generated.h"


typedef struct {
  volatile int on, note, patch, age;
  float ph[3], dt[3];
  float prev, index, fb;
  vd_env em, ec, ef;
  float lp[4];
  float pan_l, pan_r, amp, vel;
} ft_voice;

static float s_sr = 48000.0f;
static volatile int g_patch;
static ft_voice voices[FT_SYNTH_VOICES];
static int age_ctr;
static float vib_phase;
static ft_synth_ctrl live_ctrl;   /* audio flow's copy, refreshed by render */

void ft_synth_init(float sr)
{
  int i;
  s_sr = sr > 1000.0f ? sr : 48000.0f;
  g_patch = 0;
  for (i = 0; i < FT_SYNTH_VOICES; i++) {
    __atomic_store_n(&voices[i].on, 0, __ATOMIC_RELEASE);
    voices[i].age = i;
  }
  age_ctr = FT_SYNTH_VOICES;
  vib_phase = 0.0f;
}

void ft_synth_reset(void)
{
  int i;
  for (i = 0; i < FT_SYNTH_VOICES; i++)
    __atomic_store_n(&voices[i].on, 0, __ATOMIC_RELEASE);
}

void ft_synth_set_patch(int p)
{
  if (p < 0) p = 0;
  if (p >= FT_PATCH_COUNT) p = FT_PATCH_COUNT - 1;
  __atomic_store_n(&g_patch, p, __ATOMIC_RELEASE);
}

int ft_synth_patch_count(void) { return FT_PATCH_COUNT; }

const char *ft_synth_patch_name(int p)
{
  if (p < 0) p = 0;
  if (p >= FT_PATCH_COUNT) p = FT_PATCH_COUNT - 1;
  return FT_PATCHES[p].name;
}

int ft_synth_current_patch(void)
{
  return __atomic_load_n(&g_patch, __ATOMIC_ACQUIRE);
}

static float wave(float phase, float dt, int kind)
{
  if (kind == FT_WAVE_SAW) return vd_saw(phase, dt);
  if (kind == FT_WAVE_SQUARE) return vd_square(phase, dt);
  if (kind == FT_WAVE_TRI) return 2.0f * vd_absf(2.0f * phase - 1.0f) - 1.0f;
  return vd_sin(phase);
}

void ft_synth_note_on(int note, int vel, int patch)
{
  ft_voice *v = 0;
  const ft_patch *p;
  int i, best = -1, oldest = -1;
  float f, det, vs, pan, a, d, s, r;

  if (patch < 0 || patch >= FT_PATCH_COUNT) patch = 0;
  p = &FT_PATCHES[patch];

  for (i = 0; i < FT_SYNTH_VOICES; i++) {
    if (!voices[i].on) { best = i; break; }
    if (voices[i].ec.stage == 0 && voices[i].age > oldest) { oldest = voices[i].age; best = i; }
  }
  if (best < 0) {
    oldest = -1;
    for (i = 0; i < FT_SYNTH_VOICES; i++)
      if (voices[i].age > oldest) { oldest = voices[i].age; best = i; }
  }
  if (best < 0) return;
  v = &voices[best];
  __atomic_store_n(&v->on, 1, __ATOMIC_RELEASE);
  v->note = note;
  v->patch = patch;
  v->age = ++age_ctr;
  v->prev = 0.0f;
  v->amp = 0.0f;

  f = vd_mtof((float)(note + p->octave * 12));
  det = live_ctrl.detune * 0.005f;   /* +/- cents -> semitones */
  v->dt[0] = f / s_sr;
  v->dt[1] = f * (p->kind == FT_KIND_FM ? p->ratio : 1.0f) *
             vd_exp2(det / 12.0f) / s_sr;
  v->dt[2] = f * 0.5f / s_sr;
  v->ph[0] = 0.0f;
  v->ph[1] = 0.0f;
  v->ph[2] = 0.0f;
  v->index = p->index * live_ctrl.index;
  v->fb = p->fb;
  for (i = 0; i < 4; i++) v->lp[i] = 0.0f;

  vs = 1.0f - live_ctrl.vel + live_ctrl.vel * vd_clamp((float)vel / 127.0f, 0.0f, 1.0f);
  v->vel = vs * p->level;

  a = p->c_ar + 0.45f * live_ctrl.attack;
  d = p->c_dr * (0.15f + live_ctrl.decay);
  s = p->c_sl;
  r = p->c_rr + 0.45f * live_ctrl.release;
  if (p->kind == FT_KIND_ORGAN && p->c_ar <= 0.0f) {
    a = 0.005f; d = 0.15f; s = 0.95f; r = 0.15f + 0.4f * live_ctrl.release;
  }
  vd_env_set(&v->ec, a, d, s, r, s_sr);
  vd_env_set(&v->em, p->m_ar + 0.3f * live_ctrl.attack,
             p->m_dr * (0.15f + live_ctrl.decay), p->m_sl,
             p->m_rr + 0.4f * live_ctrl.release, s_sr);
  vd_env_set(&v->ef, p->fattack + live_ctrl.attack, p->fdec,
             0.0f, 0.12f + live_ctrl.release * 0.3f, s_sr);
  vd_env_gate(&v->ec, 1);
  vd_env_gate(&v->em, 1);
  vd_env_gate(&v->ef, 1);

  pan = (float)((note % 12) - 5) / 12.0f * live_ctrl.spread * 1.4f;
  pan = vd_clamp(pan, -0.95f, 0.95f);
  v->pan_l = vd_sqrt(0.5f * (1.0f - pan));
  v->pan_r = vd_sqrt(0.5f * (1.0f + pan));
}

void ft_synth_note_off(int note)
{
  int i;
  for (i = 0; i < FT_SYNTH_VOICES; i++) {
    if (voices[i].on && voices[i].note == note) {
      vd_env_gate(&voices[i].ec, 0);
      vd_env_gate(&voices[i].em, 0);
      vd_env_gate(&voices[i].ef, 0);
    }
  }
}

void ft_synth_all_off(void)
{
  int i;
  for (i = 0; i < FT_SYNTH_VOICES; i++) {
    if (voices[i].on) {
      vd_env_gate(&voices[i].ec, 0);
      vd_env_gate(&voices[i].em, 0);
      vd_env_gate(&voices[i].ef, 0);
    }
  }
}

int ft_synth_voice_count(void)
{
  int i, n = 0;
  for (i = 0; i < FT_SYNTH_VOICES; i++)
    if (__atomic_load_n(&voices[i].on, __ATOMIC_ACQUIRE)) n++;
  return n;
}

static float ladder(ft_voice *v, float in, float cut, float res)
{
  float a, x;
  int k;
  cut = vd_clamp(cut, 60.0f, s_sr * 0.40f);
  a = 1.0f - vd_coef(1.0f / (6.2831853f * cut), s_sr);
  x = in - res * 3.1f * v->lp[3];
  for (k = 0; k < 4; k++) v->lp[k] += a * (x - v->lp[k]);
  return v->lp[3];
}

void ft_synth_render(float *l, float *r, int n, const ft_synth_ctrl *c)
{
  int i, j;
  int vi;
  float vib;

  if (c) live_ctrl = *c;

  for (i = 0; i < n; i++) {
    float out_l = 0.0f, out_r = 0.0f;
    vib_phase += live_ctrl.vib_rate / s_sr;
    if (vib_phase >= 1.0f) vib_phase -= 1.0f;
    vib = 1.0f + live_ctrl.vib_depth * vd_sin(vib_phase);

    for (vi = 0; vi < FT_SYNTH_VOICES; vi++) {
      ft_voice *v = &voices[vi];
      const ft_patch *p;
      float s = 0.0f, env;
      if (!v->on) continue;
      p = &FT_PATCHES[v->patch];

      env = vd_env_next(&v->ec);
      if (v->ec.stage == 0 && env < 0.0004f) {
        __atomic_store_n(&v->on, 0, __ATOMIC_RELEASE);
        continue;
      }

      if (p->kind == FT_KIND_FM) {
        float menv = vd_env_next(&v->em);
        float mod = vd_sin(v->ph[1]) * v->index * menv;
        float ph = v->ph[0] + v->fb * v->prev + mod;
        s = vd_sin(ph);
        v->prev = s;
        s *= env;
      } else if (p->kind == FT_KIND_VA) {
        float o1, o2, sub, mix, fenv, cut;
        float d0 = v->dt[0] * vib, d1 = v->dt[1] * vib;
        v->ph[0] += d0; if (v->ph[0] >= 1.0f) v->ph[0] -= 1.0f;
        v->ph[1] += d1; if (v->ph[1] >= 1.0f) v->ph[1] -= 1.0f;
        v->ph[2] += v->dt[2]; if (v->ph[2] >= 1.0f) v->ph[2] -= 1.0f;
        o1 = wave(v->ph[0], d0, p->wave1);
        o2 = wave(v->ph[1], d1, p->wave2);
        sub = vd_sin(v->ph[2]);
        mix = (o1 + 0.72f * o2 + p->sub * 0.55f * sub) * 0.38f;
        fenv = vd_env_next(&v->ef);
        cut = p->cutoff * live_ctrl.bright * vd_exp2(p->fenv * fenv);
        s = ladder(v, mix, cut, p->res) * env;
      } else {
        float amp = env;
        amp *= 1.0f - 0.35f * vd_sin(vib_phase);
        for (j = 0; j < 6; j++) {
          float h = (float)(j + 1);
          s += p->draw[j] * vd_sin(v->ph[0] * h);
        }
        v->ph[0] += v->dt[0] * vib;
        if (v->ph[0] >= 1.0f) v->ph[0] -= 1.0f;
        s *= amp * 0.20f;
      }

      s *= v->vel;
      out_l += s * v->pan_l;
      out_r += s * v->pan_r;

      if (p->kind == FT_KIND_FM) {
        v->ph[0] += v->dt[0] * vib;
        v->ph[1] += v->dt[1] * vib;
        if (v->ph[0] >= 1.0f) v->ph[0] -= 1.0f;
        if (v->ph[1] >= 1.0f) v->ph[1] -= 1.0f;
      }
    }
    l[i] = out_l * live_ctrl.volume;
    r[i] = out_r * live_ctrl.volume;
  }
}
