#ifndef __WEUI_GFX_EFFECT_COMPOSE_H
#define __WEUI_GFX_EFFECT_COMPOSE_H

/**
 * @file weui_gfx_effect_compose.h
 * @brief What a transition's compose works with. Private to effects/, not API.
 *
 * The old picture is the outgoing one, parked in the aux frame for the whole run.
 * The new picture is the incoming one, drawn into the drawing frame every frame.
 * A compose puts the old picture back over the new one wherever the old still shows.
 *
 * Pixels are moved raw: SetPixel would put the driver brightness on them a second time.
 */

#include "def_macro.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_effect_types.h"
#include "weui_gfx_transform.h"
#include "weui_rand.h"

/** @brief Smoothstep: sets off gently and settles the same way - the default ease */
static inline float WeUi_Gfx_Effect_Smooth(float progress) {
	return progress * progress * (3.0f - 2.0f * progress);
}

/** @brief The old picture's pixel (sx, sy) at (x, y): (x, y) itself where it stands still */
static inline void WeUi_Gfx_Effect_PutOld(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y,
										  WeUi_Gfx_PtPos_t sx, WeUi_Gfx_PtPos_t sy) {
	WeUi_Gfx_Point_t dst = {.X = x, .Y = y};
	WeUi_Gfx_Point_t src = {.X = sx, .Y = sy};
	WeUi_Gfx_Adapter_SetPixelRaw(&dst, WeUi_Gfx_Adapter_GetPixelRaw_Aux(&src));
}

/** @brief The whole old picture back over the new one */
static inline void WeUi_Gfx_Effect_PutOldFrame(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)pcCtx->Height; y++)
		for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)pcCtx->Width; x++)
			WeUi_Gfx_Effect_PutOld(x, y, x, y);
}

/**
 * @brief A dark pixel, raw. The raw domain has no colour API, but the minimum channel is
 *        a fixed point of the driver's brightness LUT, so the two agree here.
 */
static inline WeUi_Gfx_ColorRaw_t WeUi_Gfx_Effect_Blank(void) {
	return WeUi_Gfx_Transform_FillBytesFromChannel(WeUi_Gfx_Adapter_GetColor_Min());
}

/** @brief `raw` lifted towards white by `shine` in [0; 1] */
static inline WeUi_Gfx_ColorRaw_t WeUi_Gfx_Effect_Lighten(WeUi_Gfx_ColorRaw_t raw, float shine) {
	WeUi_Gfx_Color_t color = WeUi_Gfx_Transform_Bytes2Color(raw);
	float max			   = (float)WeUi_Gfx_Adapter_GetColor_Max();

	color.R = (WeUi_Gfx_ColorChannel_t)(color.R + (max - color.R) * shine);
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	color.G = (WeUi_Gfx_ColorChannel_t)(color.G + (max - color.G) * shine);
	color.B = (WeUi_Gfx_ColorChannel_t)(color.B + (max - color.B) * shine);
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

	return WeUi_Gfx_Transform_Color2Bytes(color);
}

/** @brief The moment cell (a, b) takes its turn, in [0; 1): scattered, the same every run */
static inline float WeUi_Gfx_Effect_Turn(u32 a, u32 b) {
	return (float)(WeUi_Rand_Hash(a, b, 0) & 0xFFFFu) / 65536.0f;
}

/**
 * @brief How far column `x` is along its own part of the run, [0; 1]. Columns
 *        `columnPx` wide each start at a turn of their own within the first `spread`.
 */
static inline float WeUi_Gfx_Effect_ColumnRun(const WeUi_Gfx_Effect_Ctx_t* pcCtx,
											  WeUi_Gfx_PtPos_t x, u8 columnPx, float spread) {
	float start = spread * WeUi_Gfx_Effect_Turn((u32)x / GET_MAX(columnPx, 1u), 0);
	return CUT_NORM_FLOAT((pcCtx->Progress - start) / (1.0f - spread));
}

#endif /* __WEUI_GFX_EFFECT_COMPOSE_H */
