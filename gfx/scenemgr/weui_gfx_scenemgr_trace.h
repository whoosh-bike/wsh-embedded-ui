#ifndef __WEUI_GFX_SCENEMGR_TRACE_H
#define __WEUI_GFX_SCENEMGR_TRACE_H

#include "weui_gfx_scenemgr.h"

/**
 * @file weui_gfx_scenemgr_trace.h
 * @brief The scene engine's steps, for a tool that follows it - the lab draws a run's
 *        scenes and moves from them. Nothing an app needs: no callback, no cost but a
 *        NULL check per step.
 */

/** @brief What the engine just did */
typedef enum {
	WEUI_GFX_SCENEMGR_TRACE_ROUTED = 0, /* an input or a timeout answered: Scene, or SELF */
	WEUI_GFX_SCENEMGR_TRACE_SWITCH,		/* From gives way to Scene, both Exit() and Enter() done */
	WEUI_GFX_SCENEMGR_TRACE_PLAY,		/* Transition starts playing into Scene */
	WEUI_GFX_SCENEMGR_TRACE_SHOWN,		/* Scene is on the panel, its time runs */
	WEUI_GFX_SCENEMGR_TRACE_ACTION,		/* a PostAction's Action starts: the panel is its */
	WEUI_GFX_SCENEMGR_TRACE_ACTION_DONE,
} WeUi_Gfx_SceneMgr_TraceKind_t;

typedef enum {
	WEUI_GFX_SCENEMGR_CAUSE_HOME = 0, /* the first Poll() */
	WEUI_GFX_SCENEMGR_CAUSE_INPUT,
	WEUI_GFX_SCENEMGR_CAUSE_TIMEOUT,
	WEUI_GFX_SCENEMGR_CAUSE_REQUEST,
	WEUI_GFX_SCENEMGR_CAUSE_CLEAR,
	WEUI_GFX_SCENEMGR_CAUSE_HANDBACK, /* PlayIntoActive() from outside a switch */
} WeUi_Gfx_SceneMgr_Cause_t;

/**
 * @brief One step. A switch traces SWITCH, PLAY when it has a transition, then SHOWN;
 *        a hand-back PLAY and SHOWN. Scene and From are SELF where there is none.
 */
typedef struct {
	WeUi_Gfx_SceneMgr_TraceKind_t Kind;
	WeUi_Gfx_SceneMgr_Cause_t Cause;
	u32 Scene;
	u32 From;
	WeUi_Gfx_SceneMgr_TransitionFn_t Transition;
	VoidFunc_t Action;
	const void* pcInput; /* ROUTED of an input: as it was posted */
} WeUi_Gfx_SceneMgr_Trace_t;

/** @brief Called on every step from the polling context, NULL for none - the default */
void WeUi_Gfx_SceneMgr_SetTrace(void (*trace)(const WeUi_Gfx_SceneMgr_Trace_t* pcTrace));

/** @brief The engine's own: hands a step to the callback, if one is set */
void WeUi_Gfx_SceneMgr_Trace(WeUi_Gfx_SceneMgr_Trace_t trace);

#endif /* __WEUI_GFX_SCENEMGR_TRACE_H */
