#ifndef __WEUI_GFX_EFFECT_TYPES_H
#define __WEUI_GFX_EFFECT_TYPES_H

#include "weui_gfx_types.h"

/** @brief Guard against a zero-length loop, not a quality floor */
#define WEUI_GFX_EFFECT_FRAMES_MIN 1

/** @brief Which way an effect that can run either way does */
typedef enum {
	WEUI_GFX_EFFECT_AXIS_HORIZONTAL = 0,
	WEUI_GFX_EFFECT_AXIS_VERTICAL,
} WEUI_GFX_EFFECT_AXIS_t;

/** @brief Which way the moving picture of a push, swipe or cover travels */
typedef enum {
	WEUI_GFX_EFFECT_SLIDE_RIGHT = 0,
	WEUI_GFX_EFFECT_SLIDE_LEFT,
	WEUI_GFX_EFFECT_SLIDE_DOWN,
	WEUI_GFX_EFFECT_SLIDE_UP,
} WEUI_GFX_EFFECT_SLIDE_t;

/**
 * @brief Draws one frame of the incoming scene into the drawing frame. Must not
 *        push it, and must clear the frame itself - nothing seeds it.
 */
typedef void (*WeUi_Gfx_Effect_RenderCb_t)(void);

/**
 * @brief Where an effect has got to, and the panel it is playing on.
 *
 * Coordinates run [0; Width) x [0; Height): the adapter indexes the frame buffer
 * straight from these, so GetX_Min()/GetY_Min() would only look like an offset.
 *
 * ParamsPtr carries what the effect itself needs - the salt of a dissolve, the
 * direction of a push. Owned by the caller, which blocks for the whole run.
 */
typedef struct {
	u16 Frame;	/* 1..Frames */
	u16 Frames; /* clamped to WEUI_GFX_EFFECT_FRAMES_MIN */
	float Progress;
	WeUi_Gfx_ObjSize_t Width;
	WeUi_Gfx_ObjSize_t Height;
	const void* ParamsPtr;
} WeUi_Gfx_Effect_Ctx_t;

/** @brief Paint one frame of an effect */
typedef void (*WeUi_Gfx_Effect_FrameCb_t)(const WeUi_Gfx_Effect_Ctx_t* pcCtx);

/**
 * @brief Compose row `y` of a transition's frame, all of it, from the old picture and the new
 *        one. Rows come top to bottom, from 0 every frame.
 */
typedef void (*WeUi_Gfx_Effect_RowCb_t)(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y);

#endif /* __WEUI_GFX_EFFECT_TYPES_H */
