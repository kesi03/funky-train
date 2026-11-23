/* FUNKY-TRAIN -- 808/909-style drum voices and 16-step funk grooves.
 * Control flow edits patterns and reads LEDs; the audio flow triggers and
 * renders voices. No host calls, no allocation.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef FT_DRUMS_H
#define FT_DRUMS_H

#include "vibe.h"
#include "vibe_dsp.h"

#define FT_DRUM_VOICES 12
#define FT_DRUM_TRACKS 8
#define FT_DRUM_STEPS 16
#define FT_DRUM_PATTERNS 8

enum {
  FT_D_KICK = 0, FT_D_SNARE, FT_D_CLAP, FT_D_HAT,
  FT_D_OPEN, FT_D_TOM, FT_D_COW, FT_D_PERC,
  FT_D_COUNT
};

typedef struct {
  float gain, tune, decay, drive, tone;
} ft_drum_ctrl;

void  ft_drums_init(float sr);
void  ft_drums_reset(void);
void  ft_drums_trigger(int voice, int vel);       /* audio flow only */
void  ft_drums_render(float *l, float *r, int n, const ft_drum_ctrl *c);
unsigned char ft_drum_step(int pat, int step);    /* bit mask of FT_D_* tracks */
void  ft_drum_step_set(int pat, int step, int mask);
void  ft_drum_pattern_restore(int pat);
const char *ft_drum_pattern_name(int pat);
int   ft_drum_voice_count(void);

#endif
