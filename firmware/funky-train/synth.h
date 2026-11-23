/* FUNKY-TRAIN -- 80s funk keyboard engine (FM / VA / drawbar organ).
 * Audio-flow only: the control flow publishes a patch index and an fx_ctrl
 * snapshot; this file never calls host services.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef FT_SYNTH_H
#define FT_SYNTH_H

#include "vibe.h"
#include "vibe_dsp.h"

#define FT_SYNTH_VOICES 16

enum { FT_KIND_FM = 0, FT_KIND_VA, FT_KIND_ORGAN };

enum {
  FT_WAVE_SINE = 0, FT_WAVE_SAW, FT_WAVE_SQUARE, FT_WAVE_TRI
};

/* One factory voice. All times are seconds, all levels 0..1. */
typedef struct {
  const char *name;
  int kind;
  /* 2-op FM */
  float ratio, index, fb;
  float m_ar, m_dr, m_sl, m_rr;
  float c_ar, c_dr, c_sl, c_rr;
  /* VA / organ */
  int wave1, wave2, sub;
  float detune, cutoff, res, fenv, fdec, fattack;
  float draw[6];
  /* common */
  float level;
  int octave;
} ft_patch;

/* Control-flow -> audio-flow snapshot, values are plain floats. */
typedef struct {
  float index, decay, bright, detune, attack, release, vel, spread,
        vib_depth, vib_rate, volume;
} ft_synth_ctrl;

enum {
  FT_SYN_C_INDEX = 0, FT_SYN_C_DECAY, FT_SYN_C_BRIGHT, FT_SYN_C_DETUNE,
  FT_SYN_C_ATTACK, FT_SYN_C_RELEASE, FT_SYN_C_VEL, FT_SYN_C_SPREAD,
  FT_SYN_C_VIB_DEPTH, FT_SYN_C_VIB_RATE, FT_SYN_C_VOLUME,
  FT_SYN_C_COUNT
};

void  ft_synth_init(float sr);
void  ft_synth_reset(void);
void  ft_synth_set_patch(int p);
int   ft_synth_patch_count(void);
const char *ft_synth_patch_name(int p);
int   ft_synth_current_patch(void);
void  ft_synth_note_on(int note, int vel, int patch);
void  ft_synth_note_off(int note);
void  ft_synth_all_off(void);
int   ft_synth_voice_count(void);
void  ft_synth_render(float *l, float *r, int n, const ft_synth_ctrl *c);

#endif
