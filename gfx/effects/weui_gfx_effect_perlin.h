#ifndef __WEUI_GFX_EFFECT_PERLIN_H
#define __WEUI_GFX_EFFECT_PERLIN_H

#include "weui_gfx_effect_types.h"

/**
 * @brief A noise field lit through a falling threshold, so the clouds condense
 *        rather than fade in. Time is the third axis - the field lives, it does
 *        not scroll.
 */
typedef struct {
	float ScaleX;	 /* noise units per px: X coarser than Y stretches clouds */
	float ScaleY;	 /* along a wide panel */
	float Drift;	 /* noise units travelled in time */
	float Gain;		 /* 3D perlin rarely reaches +-1 on its own */
	float RevealLow; /* how far past the floor the threshold ends */
	float EdgeSoft;	 /* threshold-to-full ramp, lower is harder */
	float HueBase;
	float HueSpan;
	float HueSpin;
} WeUi_Gfx_Effect_PerlinCfg_t;

const WeUi_Gfx_Effect_PerlinCfg_t* WeUi_Gfx_Effect_PerlinCfg_Default(void);

/**
 * @brief Where the field is, as opposed to how it looks (the cfg). Paint() derives
 *        all of it from one progress value; a scene that keeps the clouds alive
 *        moves each part on its own.
 */
typedef struct {
	float Time;		 /* third noise axis: the clouds change shape as it moves */
	float Threshold; /* noise level that starts to light, ~[-RevealLow; 1]: lower shows more */
	float HueShift;	 /* degrees added to the whole field */
	float OffsetX;	 /* noise units the field is moved by; keep the */
	float OffsetY;	 /* sampled coordinates positive, perlin_3D needs that */
} WeUi_Gfx_Effect_PerlinField_t;

/**
 * @brief One frame of the field in the given state, NULL cfg taking the defaults.
 *        Covers every pixel, like Paint().
 */
void WeUi_Gfx_Effect_Perlin_PaintField(const WeUi_Gfx_Effect_PerlinField_t* pcField,
									   const WeUi_Gfx_Effect_PerlinCfg_t* pcCfg);

/**
 * @brief One frame at the given progress, NULL cfg taking the defaults. Covers
 *        every pixel, so a scene can call it straight without clearing first.
 */
void WeUi_Gfx_Effect_Perlin_Paint(float progress, const WeUi_Gfx_Effect_PerlinCfg_t* pcCfg);

/** @brief The whole animation, condensing out of the dark over `frames` */
RET_STATE_t WeUi_Gfx_Effect_Perlin_Play(u16 frames, const WeUi_Gfx_Effect_PerlinCfg_t* pcCfg);

#endif /* __WEUI_GFX_EFFECT_PERLIN_H */
