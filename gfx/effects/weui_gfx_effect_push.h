#ifndef __WEUI_GFX_EFFECT_PUSH_H
#define __WEUI_GFX_EFFECT_PUSH_H

#include "weui_gfx_effect_types.h"

/**
 * @brief Shape of the throw: out to a peak past the edge, then swings back and
 *        forth across it, each smaller than the last, every swing given the same
 *        time as a spring's half-periods are. The peak scales with the travel, as a
 *        real spring's does, inside a px band that keeps it readable on any panel.
 */
typedef struct {
	float OvershootShare; /* of the travel, how far past the edge the first peak goes */
	float OvershootMinPx; /* floor: below it the bounce stops reading */
	float OvershootMaxPx; /* ceiling: past it a wide panel flies rather than bounces */
	float ApproachShare;  /* of the frames, spent reaching the first peak: 1 never swings */
	float BounceKeep;	  /* each swing's amplitude as a share of the one before */
	u8 Bounces;			  /* swings back across the edge after the first peak, at most:
						   * one under 2 px is skipped */
} WeUi_Gfx_Effect_PushCfg_t;

/** @brief Tuned defaults, and the starting point for a panel-specific copy */
const WeUi_Gfx_Effect_PushCfg_t* WeUi_Gfx_Effect_PushCfg_Default(void);

/**
 * @brief Slide the old picture off the panel, the incoming scene in behind it.
 *        The slide overshoots the edge and springs back, as a swipe rubber-bands.
 *
 * @param frames  a push eases, so it travels several pixels a frame at its
 *                fastest - one frame per pixel of travel is where that shows
 * @param pcCfg   NULL takes WeUi_Gfx_Effect_PushCfg_Default(). Not copied.
 */
RET_STATE_t WeUi_Gfx_Effect_Push(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								 WEUI_GFX_EFFECT_SLIDE_t slide,
								 const WeUi_Gfx_Effect_PushCfg_t* pcCfg);

/** @brief A push without the bounce: thrown in, braking onto the edge and stopping there */
RET_STATE_t WeUi_Gfx_Effect_Swipe(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								  WEUI_GFX_EFFECT_SLIDE_t slide);

#endif /* __WEUI_GFX_EFFECT_PUSH_H */
