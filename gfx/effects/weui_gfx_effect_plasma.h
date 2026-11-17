#ifndef __WEUI_GFX_EFFECT_PLASMA_H
#define __WEUI_GFX_EFFECT_PLASMA_H

#include "weui_gfx_effect_types.h"

/**
 * @brief Three sine waves beating against each other, their sum driving hue and
 *        brightness together. Drift rates must differ, or the waves stand still.
 */
typedef struct {
	float WaveX; /* rad per px */
	float WaveY;
	float WaveDiag;
	float DriftDeg;	 /* wave travel over the animation */
	float DriftY;	 /* relative to the above */
	float DriftDiag; /* likewise */
	float Bloom;	 /* share spent rising out of the dark */
	float HueBase;
	float HueSpan;
	float HueSpin;	/* degrees the field turns over the animation */
	float BriFloor; /* troughs stay lit, or the field breaks up */
} WeUi_Gfx_Effect_PlasmaCfg_t;

const WeUi_Gfx_Effect_PlasmaCfg_t* WeUi_Gfx_Effect_PlasmaCfg_Default(void);

/**
 * @brief One frame at the given progress, NULL cfg taking the defaults. Covers
 *        every pixel, so a scene can call it straight without clearing first.
 */
void WeUi_Gfx_Effect_Plasma_Paint(float progress, const WeUi_Gfx_Effect_PlasmaCfg_t* pcCfg);

/** @brief The whole animation, blooming out of the dark over `frames` */
RET_STATE_t WeUi_Gfx_Effect_Plasma_Play(u16 frames, const WeUi_Gfx_Effect_PlasmaCfg_t* pcCfg);

#endif /* __WEUI_GFX_EFFECT_PLASMA_H */
