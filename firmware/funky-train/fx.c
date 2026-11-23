/* FUNKY-TRAIN -- 80s funk effects. Clean-room DSP, no samples, no libc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "fx.h"
#include "vibe_dsp.h"

#define FXBUF 8192
#define FXDLY 32768
#define FXCOMB 4096
#define FXAP 512

typedef struct { float b0, b1, b2, a1, a2; } biq;
typedef struct { float z1, z2; } biqz;

static float sr;
static volatile int wah_hz100, rotor_hz100;

static biq beq[3];
static biqz eqz[3][2];
static float sat_lp, comp_env, comp_gain;
static vd_svf wah_svf, cross;
static float wah_cut, wah_env, wah_hold, wah_phase;

static float dim_buf[2][FXBUF], ch_buf[2][FXBUF];
static int dim_w, ch_w;
static float dim_phase, ch_phase, ph_phase;
static float ph_state[2][4];

static float rot_hz, rot_target, rot_phase_h, rot_phase_d;
static float rot_wet, rot_hlx, rot_dlx;
static float horn_buf[FXBUF], drum_buf[FXBUF];
static int rot_w;

static float dly[2][FXDLY], dly_lp[2];
static int dly_w;

static float comb[4][2][FXCOMB], comb_lp[4][2];
static int comb_w[4][2];
static float ap[2][2][FXAP];
static int ap_w[2][2];

static float tone_lp_l, tone_lp_r;
static vd_noise fxn;

static float q(int v) { return (float)v * 0.001f; }
static float cl(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

static void biq_set(biq *c, int type, float hz, float qq, float db, float s)
{
  float A, w, sn, cs, al, a0, b0, b1, b2, a1, a2;
  hz = cl(hz, 20.0f, s * 0.45f);
  A = vd_exp2(db / 40.0f);
  w = hz / s;
  sn = vd_sin(w); cs = vd_sin(w + 0.25f);
  al = sn / (2.0f * (qq > 0.05f ? qq : 0.05f));
  if (type == 0) { /* low shelf */
    float t = 2.0f * vd_sqrt(A) * al;
    a0 = 1.0f + t + (A - 1.0f) * cs + (A + 1.0f - (A - 1.0f) * cs) * A; /* unused */
    b0 = A * ((A + 1.0f) - (A - 1.0f) * cs + t);
    b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs);
    b2 = A * ((A + 1.0f) - (A - 1.0f) * cs - t);
    a0 = (A + 1.0f) + (A - 1.0f) * cs + t;
    a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cs);
    a2 = (A + 1.0f) + (A - 1.0f) * cs - t;
  } else if (type == 2) { /* high shelf */
    float t = 2.0f * vd_sqrt(A) * al;
    b0 = A * ((A + 1.0f) + (A - 1.0f) * cs + t);
    b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs);
    b2 = A * ((A + 1.0f) + (A - 1.0f) * cs - t);
    a0 = (A + 1.0f) - (A - 1.0f) * cs + t;
    a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cs);
    a2 = (A + 1.0f) - (A - 1.0f) * cs - t;
  } else { /* peaking */
    b0 = 1.0f + al * A;
    b1 = -2.0f * cs;
    b2 = 1.0f - al * A;
    a0 = 1.0f + al / A;
    a1 = -2.0f * cs;
    a2 = 1.0f - al / A;
  }
  if (a0 < 1e-6f) a0 = 1e-6f;
  c->b0 = b0 / a0; c->b1 = b1 / a0; c->b2 = b2 / a0;
  c->a1 = a1 / a0; c->a2 = a2 / a0;
}

static float biq_tick(biq *c, biqz *z, float x)
{
  float y = c->b0 * x + z->z1;
  z->z1 = c->b1 * x - c->a1 * y + z->z2;
  z->z2 = c->b2 * x - c->a2 * y;
  return y;
}

static float line_at(float *buf, int w, int back)
{
  int i = w - back;
  while (i < 0) i += FXBUF;
  while (i >= FXBUF) i -= FXBUF;
  return buf[i];
}

void ft_fx_init(float s)
{
  sr = s > 1000.0f ? s : 48000.0f;
  ft_fx_reset();
  fxn.s = 0xA5A5A5A5u;
}

void ft_fx_reset(void)
{
  int i, j, k;
  for (i = 0; i < 3; i++) { beq[i].b0 = 1; beq[i].b1 = beq[i].b2 = beq[i].a1 = beq[i].a2 = 0;
    for (j = 0; j < 2; j++) eqz[i][j].z1 = eqz[i][j].z2 = 0; }
  sat_lp = comp_env = comp_gain = 0;
  wah_svf.ic1 = wah_svf.ic2 = 0;
  cross.ic1 = cross.ic2 = 0;
  wah_cut = 400; wah_env = 0; wah_hold = 0; wah_phase = 0;
  for (i = 0; i < 2; i++) { for (j = 0; j < FXBUF; j++) { dim_buf[i][j] = 0; ch_buf[i][j] = 0; } }
  dim_w = ch_w = 0; dim_phase = ch_phase = ph_phase = 0;
  for (i = 0; i < 2; i++) for (j = 0; j < 4; j++) ph_state[i][j] = 0;
  rot_hz = rot_target = 0; rot_phase_h = rot_phase_d = 0; rot_wet = 0; rot_hlx = rot_dlx = 0;
  for (j = 0; j < FXBUF; j++) { horn_buf[j] = 0; drum_buf[j] = 0; }
  rot_w = 0;
  for (i = 0; i < 2; i++) { for (j = 0; j < FXDLY; j++) dly[i][j] = 0; dly_lp[i] = 0; }
  dly_w = 0;
  for (i = 0; i < 4; i++) { comb_lp[i][0] = comb_lp[i][1] = 0;
    for (j = 0; j < 2; j++) { comb_w[i][j] = 0; for (k = 0; k < FXCOMB; k++) comb[i][j][k] = 0; } }
  for (i = 0; i < 2; i++) for (j = 0; j < 2; j++) {
    ap_w[i][j] = 0; for (k = 0; k < FXAP; k++) ap[i][j][k] = 0; }
  tone_lp_l = tone_lp_r = 0;
  __atomic_store_n(&wah_hz100, 0, __ATOMIC_RELEASE);
  __atomic_store_n(&rotor_hz100, 0, __ATOMIC_RELEASE);
}

int ft_fx_wah_hz100(void) { return __atomic_load_n(&wah_hz100, __ATOMIC_ACQUIRE); }
int ft_fx_rotor_hz100(void) { return __atomic_load_n(&rotor_hz100, __ATOMIC_ACQUIRE); }

static float allpass_tick(float *buf, int *w, float x, float g)
{
  int r = *w;
  float b = buf[r], y = -g * x + b;
  buf[r] = x + g * y;
  if (++(*w) >= FXAP) *w = 0;
  return y;
}

void ft_fx_process(float *l, float *r, int n, const int *a)
{
  int i;
  if (!a || !l || !r || n <= 0) return;
  float master = q(a[FX_MASTER]);
  float drive = q(a[FX_DRIVE]);
  float sat_type = q(a[FX_SAT_TYPE]);
  float comp = q(a[FX_COMP]);
  float wmode = q(a[FX_WAH_MODE]);
  float wspeed = q(a[FX_WAH_SPEED]);
  float wdepth = q(a[FX_WAH_DEPTH]);
  float wres = q(a[FX_WAH_RES]);
  float wlo = q(a[FX_WAH_LOW]), whi = q(a[FX_WAH_HIGH]);
  float wmix = q(a[FX_WAH_MIX]);
  float chmix = q(a[FX_CHORUS]);
  float dim = q(a[FX_DIM]);
  float phmix = q(a[FX_PHASER]);
  float ron = q(a[FX_ROTARY]), rspeed = q(a[FX_ROTARY_SPEED]), rmix = q(a[FX_ROTARY_MIX]);
  float dmix = q(a[FX_DELAY_MIX]), dms = q(a[FX_DELAY_MS]), dfb = q(a[FX_DELAY_FB]);
  float vmix = q(a[FX_REVERB_MIX]), vroom = q(a[FX_REVERB_ROOM]), gate = q(a[FX_GATE]);
  float ratio = vd_log2(whi / wlo);
  float w_slew = vd_coef(0.0025f, sr);
  float dly_n = cl(dms * sr / 1000.0f, 4.0f, (float)FXDLY - 8.0f);
  int dly_back = (int)dly_n;
  float dly_frac = dly_n - (float)dly_back;
  float fb = cl(dfb, 0.0f, 0.88f);
  float damp = vd_coef(0.006f + 0.02f * (1.0f - vroom), sr);
  float decay = 0.972f + 0.024f * vroom;
  float apg = 0.55f;
  float rot_goal = ron > 0.5f ? (rspeed > 1.5f ? 6.6f : (rspeed > 0.5f ? 0.85f : 0.0f)) : 0.0f;
  float motor = vd_coef(rot_goal > rot_hz ? 0.35f : (rot_goal > 0.1f ? 0.60f : 0.22f), sr);

  biq_set(&beq[0], 0, 150.0f, 0.71f, q(a[FX_EQ_LOW]), sr);
  biq_set(&beq[1], 1, 950.0f, 0.80f, q(a[FX_EQ_MID]), sr);
  biq_set(&beq[2], 2, 5200.0f, 0.71f, q(a[FX_EQ_HIGH]), sr);

  for (i = 0; i < n; i++) {
    float x = 0.5f * (l[i] + r[i]);
    float xl = l[i], xr = r[i];
    float in_env = vd_absf(x);
    float wetl = 0, wetr = 0;
    float wah_cut_t, pos;
    vd_svf_out sv;
    int k;

    /* tape saturation: asymmetric soft clip with a little head scrape */
    sat_lp += 0.08f * (x - sat_lp);
    if (sat_type < 0.5f) {
      xl = vd_tanh(xl * (1.0f + 5.0f * drive)) * 0.87f;
      xr = vd_tanh(xr * (1.0f + 5.0f * drive)) * 0.87f;
    } else {
      xl = vd_tanh((xl + 0.25f * sat_lp) * (1.0f + 7.0f * drive)) * 0.82f;
      xr = vd_tanh((xr - 0.25f * sat_lp) * (1.0f + 7.0f * drive)) * 0.82f;
    }

    /* bus compressor */
    if (in_env > comp_env) comp_env += (in_env - comp_env) * vd_coef(0.004f, sr);
    else comp_env += (in_env - comp_env) * vd_coef(0.10f, sr);
    comp_gain = 1.0f / (1.0f + comp * 8.0f * cl(comp_env - 0.12f, 0.0f, 2.0f));
    xl *= comp_gain; xr *= comp_gain;

    for (k = 0; k < 3; k++) {
      xl = biq_tick(&beq[k], &eqz[k][0], xl);
      xr = biq_tick(&beq[k], &eqz[k][1], xr);
    }

    /* auto wah / envelope filter */
    if (wmode > 0.5f) {
      float target;
      if (wmode < 1.5f) {
        wah_phase += wspeed * 8.0f / sr;
        if (wah_phase >= 1.0f) wah_phase -= 1.0f;
        target = 0.5f + 0.5f * vd_sin(wah_phase);
      } else {
        float att = vd_coef(0.012f, sr), rel = vd_coef(0.14f, sr);
        if (in_env > wah_env) wah_env += (in_env - wah_env) * att;
        else wah_env += (in_env - wah_env) * rel;
        if (wah_env > 0.05f) wah_hold = vd_coef(0.05f, sr) * wah_hold + 0.06f;
        else wah_hold *= vd_coef(0.22f, sr);
        target = cl(2.6f * (wah_env + 0.5f * wah_hold) * wdepth, 0.0f, 1.0f);
      }
      pos = cl(target, 0.0f, 1.0f);
      pos = pos * pos * (3.0f - 2.0f * pos);
      wah_cut_t = wlo * vd_exp2(pos * ratio);
    } else {
      wah_cut_t = 12000.0f;
    }
    wah_cut += (wah_cut_t - wah_cut) * w_slew;
    __atomic_store_n(&wah_hz100, (int)(wah_cut * 100.0f), __ATOMIC_RELEASE);
    sv = vd_svf_run(&wah_svf, 0.5f * (xl + xr), wah_cut, wres, sr);
    if (wmode > 0.5f) {
      float w = wmix;
      xl = xl * (1.0f - w) + sv.bp * w;
      xr = xr * (1.0f - w) + sv.bp * w;
    }

    /* Dimension D: two short, cross-fed modulated delays */
    if (dim > 0.001f) {
      float a1 = 0.00028f + 0.00012f * vd_sin(dim_phase);
      float a2 = 0.00062f + 0.00016f * vd_sin(dim_phase + 0.31f);
      int b1 = (int)(a1 * sr) + 1, b2 = (int)(a2 * sr) + 1;
      float dl = line_at(dim_buf[0], dim_w, b1), dr = line_at(dim_buf[1], dim_w, b2);
      wetl = dr * dim; wetr = dl * dim;
      dim_buf[0][dim_w] = xl; dim_buf[1][dim_w] = xr;
      if (++dim_w >= FXBUF) dim_w = 0;
      dim_phase += 0.21f / sr; if (dim_phase >= 1.0f) dim_phase -= 1.0f;
    }

    /* stereo chorus */
    if (chmix > 0.001f) {
      float d = (0.011f + 0.007f * vd_sin(ch_phase)) * sr;
      int b = 1 + (int)d;
      float fl = d - (float)b;
      float l0 = line_at(ch_buf[0], ch_w, b), l1 = line_at(ch_buf[0], ch_w, b + 1);
      float r0 = line_at(ch_buf[1], ch_w, b), r1 = line_at(ch_buf[1], ch_w, b + 1);
      wetl += (l0 + (l1 - l0) * fl) * chmix;
      wetr += (r0 + (r1 - r0) * fl) * chmix;
      ch_buf[0][ch_w] = xl; ch_buf[1][ch_w] = xr;
      if (++ch_w >= FXBUF) ch_w = 0;
      ch_phase += 0.38f / sr; if (ch_phase >= 1.0f) ch_phase -= 1.0f;
    }
    xl += wetl; xr += wetr;

    /* four-stage phaser: cascaded first-order allpasses */
    if (phmix > 0.001f) {
      float g, dryl = xl, dryr = xr;
      ph_phase += 0.55f / sr; if (ph_phase >= 1.0f) ph_phase -= 1.0f;
      g = 0.62f + 0.22f * vd_sin(ph_phase);
      for (k = 0; k < 4; k++) {
        float gk = g * (1.0f - 0.09f * (float)k);
        float yl = -gk * xl + ph_state[0][k];
        ph_state[0][k] = xl + gk * yl;
        xl = yl;
        float yr = -gk * xr + ph_state[1][k];
        ph_state[1][k] = xr + gk * yr;
        xr = yr;
      }
      xl = dryl * (1.0f - phmix) + xl * phmix;
      xr = dryr * (1.0f - phmix) + xr * phmix;
    }

    /* rotary speaker: crossover + two Doppler-modulated rotor taps */
    rot_hz += (rot_goal - rot_hz) * motor;
    __atomic_store_n(&rotor_hz100, (int)(rot_hz * 100.0f), __ATOMIC_RELEASE);
    if (rmix > 0.001f) {
      sv = vd_svf_run(&cross, x, 800.0f, 0.71f, sr);
      float horn = sv.hp, drum = sv.lp;
      float dt_h = 0.0009f + 0.00045f * vd_sin(rot_phase_h);
      float dt_d = 0.0021f + 0.00085f * vd_sin(rot_phase_d);
      int bh = 1 + (int)(dt_h * sr), bd = 1 + (int)(dt_d * sr);
      float hl = line_at(horn_buf, rot_w, bh), hr = line_at(horn_buf, rot_w, bh + 2);
      float dl = line_at(drum_buf, rot_w, bd), dr2 = line_at(drum_buf, rot_w, bd + 2);
      horn_buf[rot_w] = vd_tanh(horn * 1.3f);
      drum_buf[rot_w] = drum;
      if (++rot_w >= FXBUF) rot_w = 0;
      rot_hlx = 0.90f * hl + 0.14f * hr;
      rot_dlx = 0.90f * dl + 0.14f * dr2;
      rot_phase_h += rot_hz / sr; if (rot_phase_h >= 1.0f) rot_phase_h -= 1.0f;
      rot_phase_d += rot_hz * 0.83f / sr; if (rot_phase_d >= 1.0f) rot_phase_d -= 1.0f;
      rot_wet += (rmix - rot_wet) * vd_coef(0.05f, sr);
      wetl = (0.45f * rot_hlx + 0.55f * rot_dlx - 0.5f * (xl + xr)) * rot_wet;
      wetr = (0.55f * rot_hlx + 0.45f * rot_dlx - 0.5f * (xl + xr)) * rot_wet;
      xl += wetl; xr += wetr;
    } else {
      rot_wet += (0.0f - rot_wet) * vd_coef(0.05f, sr);
    }

    /* analog-style ping-pong delay */
    if (dmix > 0.001f) {
      float din = 0.5f * (xl + xr) + fb * 0.5f * (dly_lp[0] + dly_lp[1]);
      int r0 = dly_w - dly_back, r1;
      float a, b;
      while (r0 < 0) r0 += FXDLY;
      r1 = r0 + 1; if (r1 >= FXDLY) r1 = 0;
      a = dly[0][r0] + (dly[0][r1] - dly[0][r0]) * dly_frac;
      b = dly[1][r0] + (dly[1][r1] - dly[1][r0]) * dly_frac;
      dly_lp[0] += damp * (b - dly_lp[0]);
      dly_lp[1] += damp * (a - dly_lp[1]);
      dly[0][dly_w] = din;
      dly[1][dly_w] = dly_lp[0];
      if (++dly_w >= FXDLY) dly_w = 0;
      xl += a * dmix; xr += b * dmix;
    }

    /* gated plate reverb */
    if (vmix > 0.001f) {
      float in = 0.5f * (xl + xr);
      float rev = 0.0f;
      int c;
      for (c = 0; c < 4; c++) {
        float y = comb[c][0][comb_w[c][0]];
        float lp = comb_lp[c][0] + decay * (y - comb_lp[c][0]);
        comb_lp[c][0] = lp;
        comb[c][0][comb_w[c][0]] = in + 0.78f * lp;
        if (++comb_w[c][0] >= FXCOMB) comb_w[c][0] = 0;
        rev += (c & 1) ? -y * 0.25f : y * 0.25f;
        y = comb[c][1][comb_w[c][1]];
        lp = comb_lp[c][1] + decay * (y - comb_lp[c][1]);
        comb_lp[c][1] = lp;
        comb[c][1][comb_w[c][1]] = in + 0.78f * lp;
        if (++comb_w[c][1] >= FXCOMB) comb_w[c][1] = 0;
        rev += (c & 1) ? y * 0.25f : -y * 0.25f;
      }
      for (c = 0; c < 2; c++) {
        rev = allpass_tick(ap[0][c], &ap_w[0][c], rev, apg);
      }
      if (gate > 0.01f) {
        float gth = 0.02f + 0.12f * gate;
        float g = in_env > gth ? 1.0f : vd_coef(0.055f, sr);
        rev *= g;
      }
      xl += rev * vmix; xr += rev * vmix;
    }

    tone_lp_l += 0.55f * (xl - tone_lp_l);
    tone_lp_r += 0.55f * (xr - tone_lp_r);
    l[i] = vd_tanh((0.7f * xl + 0.3f * tone_lp_l) * master);
    r[i] = vd_tanh((0.7f * xr + 0.3f * tone_lp_r) * master);
    l[i] = cl(l[i], -1.0f, 1.0f);
    r[i] = cl(r[i], -1.0f, 1.0f);
  }
}
