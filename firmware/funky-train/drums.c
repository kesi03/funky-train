/* FUNKY-TRAIN -- synthesized 808/909 funk drums.
 * Kick, snare, clap, hats, tom, cowbell and percussion, plus eight original
 * 16-step grooves. Everything is generated: no samples, no impulse responses.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "drums.h"

#define BIT(t) (1u << (t))

static const char *const PAT_NAMES[FT_DRUM_PATTERNS] = {
  "DAH FUNK", "BOOGIE", "ELECTRO", "GAP BAND",
  "CAMEO", "SLOW JAM", "THE BREAK", "FUNK DUB"
};

/* bit 0 kick, 1 snare, 2 clap, 3 hat, 4 open hat, 5 tom, 6 cowbell, 7 perc */
static const unsigned char FACTORY[FT_DRUM_PATTERNS][FT_DRUM_STEPS] = {
  { 0x09,0x08,0x0A,0x08, 0x02,0x08,0x09,0x18, 0x01,0x08,0x0A,0x08, 0x02,0x08,0x49,0x18 },
  { 0x01,0x08,0x09,0x08, 0x02,0x08,0x01,0x18, 0x01,0x08,0x09,0x08, 0x22,0x08,0x01,0x18 },
  { 0x01,0x08,0x41,0x08, 0x04,0x08,0x41,0x18, 0x01,0x08,0x41,0x08, 0x04,0x08,0x61,0x18 },
  { 0x01,0x00,0x09,0x08, 0x02,0x08,0x00,0x18, 0x01,0x08,0x09,0x00, 0x02,0x88,0x00,0x18 },
  { 0x01,0x08,0x09,0x08, 0x04,0x08,0x09,0x18, 0x01,0x08,0x09,0x08, 0x04,0x28,0x41,0x18 },
  { 0x01,0x00,0x00,0x08, 0x02,0x00,0x08,0x00, 0x01,0x08,0x00,0x08, 0x02,0x00,0x48,0x10 },
  { 0x01,0x08,0x09,0x08, 0x02,0x08,0x89,0x18, 0x01,0x08,0x09,0x08, 0x22,0x88,0x41,0x18 },
  { 0x01,0x08,0x00,0x08, 0x04,0x00,0x08,0x10, 0x01,0x08,0x00,0x08, 0x04,0x20,0x08,0x10 }
};

typedef struct {
  volatile int on, kind, age;   /* age is the allocation stamp, used for voice stealing */
  int samples;                  /* samples since trigger, used for transients */
  float ph, ph2, env, dec, lp, hp_lp, f0, f1, pan_l, pan_r, vel;
  vd_noise noise;
} ft_dvoice;

static float s_sr;
static ft_dvoice dv[FT_DRUM_VOICES];
static volatile unsigned char steps[FT_DRUM_PATTERNS][FT_DRUM_STEPS];
static int age_ctr;

void ft_drums_init(float sr)
{
  int p, s;
  s_sr = sr > 1000.0f ? sr : 48000.0f;
  for (p = 0; p < FT_DRUM_PATTERNS; p++)
    for (s = 0; s < FT_DRUM_STEPS; s++) steps[p][s] = FACTORY[p][s];
  for (s = 0; s < FT_DRUM_VOICES; s++) { dv[s].on = 0; dv[s].samples = 0; }
  age_ctr = 0;
  VIBE_RELEASE();
}

void ft_drums_reset(void)
{
  int s;
  for (s = 0; s < FT_DRUM_VOICES; s++) dv[s].on = 0;
}

void ft_drum_step_set(int pat, int step, int mask)
{
  if (pat < 0 || pat >= FT_DRUM_PATTERNS) return;
  if (step < 0 || step >= FT_DRUM_STEPS) return;
  steps[pat][step] = (unsigned char)(mask & 255);
  VIBE_RELEASE();
}

void ft_drum_pattern_restore(int pat)
{
  int s;
  if (pat < 0 || pat >= FT_DRUM_PATTERNS) return;
  for (s = 0; s < FT_DRUM_STEPS; s++) steps[pat][s] = FACTORY[pat][s];
  VIBE_RELEASE();
}

unsigned char ft_drum_step(int pat, int step)
{
  unsigned char m;
  if (pat < 0 || pat >= FT_DRUM_PATTERNS) pat = 0;
  if (step < 0 || step >= FT_DRUM_STEPS) step = 0;
  VIBE_ACQUIRE();
  m = steps[pat][step];
  return m;
}

const char *ft_drum_pattern_name(int pat)
{
  if (pat < 0 || pat >= FT_DRUM_PATTERNS) pat = 0;
  return PAT_NAMES[pat];
}

int ft_drum_voice_count(void)
{
  int i, n = 0;
  for (i = 0; i < FT_DRUM_VOICES; i++) if (dv[i].on) n++;
  return n;
}

void ft_drums_trigger(int voice, int vel)
{
  ft_dvoice *v = 0;
  int i, best = -1, oldest = -1;
  float amp = vd_clamp((float)vel / 127.0f, 0.05f, 1.0f);

  if (voice < 0 || voice >= FT_D_COUNT) voice = FT_D_KICK;
  for (i = 0; i < FT_DRUM_VOICES; i++) {
    if (!dv[i].on) { best = i; break; }
    if (dv[i].age > oldest) { oldest = dv[i].age; best = i; }
  }
  if (best < 0) return;
  v = &dv[best];
  v->on = 1;
  v->kind = voice;
  v->age = ++age_ctr;
  v->samples = 0;
  v->env = 1.0f;
  v->vel = amp;
  v->ph = 0.0f;
  v->ph2 = 0.0f;
  v->lp = 0.0f;
  v->hp_lp = 0.0f;
  v->noise.s = 0x9E3779B9u + (uint32_t)(age_ctr * 2654435761u);
  switch (voice) {
  case FT_D_KICK:   v->f0 = 150.f; v->f1 = 46.f;  v->dec = 0.36f; v->pan_l = v->pan_r = .72f; break;
  case FT_D_SNARE:  v->f0 = 210.f; v->f1 = 165.f; v->dec = 0.19f; v->pan_l = .78f; v->pan_r = .66f; break;
  case FT_D_CLAP:   v->f0 = 0.f;   v->f1 = 0.f;   v->dec = 0.26f; v->pan_l = .66f; v->pan_r = .80f; break;
  case FT_D_HAT:    v->f0 = 0.f;   v->f1 = 0.f;   v->dec = 0.055f; v->pan_l = .82f; v->pan_r = .58f; break;
  case FT_D_OPEN:   v->f0 = 0.f;   v->f1 = 0.f;   v->dec = 0.42f; v->pan_l = .58f; v->pan_r = .82f; break;
  case FT_D_TOM:    v->f0 = 210.f; v->f1 = 96.f;  v->dec = 0.30f; v->pan_l = .70f; v->pan_r = .70f; break;
  case FT_D_COW:    v->f0 = 540.f; v->f1 = 800.f; v->dec = 0.20f; v->pan_l = .68f; v->pan_r = .72f; break;
  default:          v->f0 = 0.f;   v->f1 = 0.f;   v->dec = 0.09f; v->pan_l = .72f; v->pan_r = .68f; break;
  }
}

void ft_drums_render(float *l, float *r, int n, const ft_drum_ctrl *c)
{
  int i, vi;
  float gain = c ? c->gain : 0.8f;
  float tune = c ? vd_clamp(c->tune, 0.5f, 1.8f) : 1.0f;
  float decs = c ? vd_clamp(c->decay, 0.25f, 2.5f) : 1.0f;
  float drive = c ? vd_clamp(c->drive, 0.0f, 1.0f) : 0.0f;
  float tone = c ? vd_clamp(c->tone, 0.0f, 1.0f) : 0.5f;

  for (i = 0; i < n; i++) {
    float out_l = 0.0f, out_r = 0.0f;
    for (vi = 0; vi < FT_DRUM_VOICES; vi++) {
      ft_dvoice *v = &dv[vi];
      float s = 0.0f, env;
      if (!v->on) continue;
      v->samples++;
      env = v->env;
      if (env < 0.0006f) { v->on = 0; continue; }

      if (v->kind == FT_D_KICK || v->kind == FT_D_TOM) {
        float f = (v->f1 + (v->f0 - v->f1) * env * env) * tune;
        v->ph += vd_clamp(f, 25.f, 1000.f) / s_sr;
        if (v->ph >= 1.0f) v->ph -= 1.0f;
        s = vd_sin(v->ph) * env;
        if (v->kind == FT_D_KICK && v->samples < (int)(s_sr * 0.004f))
          s += vd_noise_next(&v->noise) * 0.35f * (1.0f - (float)v->samples / (s_sr * 0.004f));
      } else if (v->kind == FT_D_SNARE) {
        float f = v->f1 * tune;
        float nz = vd_noise_next(&v->noise);
        v->ph += vd_clamp(f, 40.f, 1000.f) / s_sr;
        if (v->ph >= 1.0f) v->ph -= 1.0f;
        v->hp_lp += 0.36f * (nz - v->hp_lp);
        s = (0.55f * (nz - v->hp_lp) + 0.45f * vd_sin(v->ph)) * env;
      } else if (v->kind == FT_D_COW) {
        float dt1 = v->f0 * tune / s_sr, dt2 = v->f1 * tune / s_sr;
        v->ph += dt1; v->ph2 += dt2;
        if (v->ph >= 1.0f) v->ph -= 1.0f;
        if (v->ph2 >= 1.0f) v->ph2 -= 1.0f;
        v->lp += 0.30f * (vd_square(v->ph, dt1) + vd_square(v->ph2, dt2) - v->lp);
        s = v->lp * env * 0.45f;
      } else {
        float nz = vd_noise_next(&v->noise);
        int clap = v->kind == FT_D_CLAP;
        int open = v->kind == FT_D_OPEN;
        int gate = 1;
        if (clap) {
          int a = v->samples;
          int b1 = (int)(s_sr * 0.010f), b2 = (int)(s_sr * 0.021f);
          gate = (a < (int)(s_sr * 0.002f) || (a >= b1 && a < b1 + (int)(s_sr * 0.002f)) ||
                  (a >= b2 && a < b2 + (int)(s_sr * 0.002f)) || a > b2) ? 1 : 0;
          env *= gate ? 1.0f : 0.55f;
        }
        v->hp_lp += (open ? 0.28f : (0.25f + 0.45f * tone)) * (nz - v->hp_lp);
        s = (nz - v->hp_lp) * env * (clap ? 0.75f : 0.60f);
      }

      if (drive > 0.0f) s = vd_tanh(s * (1.0f + 4.0f * drive));
      s *= v->vel;
      out_l += s * v->pan_l;
      out_r += s * v->pan_r;
      v->env *= vd_coef(v->dec / decs / 5.0f, s_sr);
    }
    l[i] = out_l * gain;
    r[i] = out_r * gain;
  }
}
