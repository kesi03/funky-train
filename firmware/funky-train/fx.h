/* FUNKY-TRAIN -- essential 80s funk FX rack.
 * tape sat -> comp -> EQ -> auto wah / envelope filter -> Dimension D +
 * chorus -> phaser -> rotary -> analog ping-pong delay -> gated reverb.
 * Audio-flow only; amounts are the x1000 parameter ints published by main.c.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef FT_FX_H
#define FT_FX_H

enum {
  FX_SAT_TYPE = 0, FX_DRIVE, FX_COMP,
  FX_EQ_LOW, FX_EQ_MID, FX_EQ_HIGH,
  FX_WAH_MODE, FX_WAH_SPEED, FX_WAH_DEPTH, FX_WAH_RES,
  FX_WAH_LOW, FX_WAH_HIGH, FX_WAH_MIX,
  FX_CHORUS, FX_DIM, FX_PHASER,
  FX_ROTARY, FX_ROTARY_SPEED, FX_ROTARY_MIX,
  FX_DELAY_MIX, FX_DELAY_MS, FX_DELAY_FB,
  FX_REVERB_MIX, FX_REVERB_ROOM, FX_GATE,
  FX_MASTER,
  FX_COUNT
};

void ft_fx_init(float sr);
void ft_fx_reset(void);
void ft_fx_process(float *l, float *r, int n, const int *amounts);
int  ft_fx_wah_hz100(void);
int  ft_fx_rotor_hz100(void);

#endif
