#include "weui_gfx_scenemgr.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_fps_control.h"
#include "weui_gfx_scenemgr_trace.h"

#ifndef WEUI_ENTER_CRITICAL
#warning WEUI_ENTER_CRITICAL is not defined - posting a message is not interrupt-safe
#define WEUI_CRITICAL_DECL()
#define WEUI_ENTER_CRITICAL()
#define WEUI_EXIT_CRITICAL()
#endif /* WEUI_ENTER_CRITICAL */

#define WEUI_GFX_SCENEMGR_MS_IN_SEC 1000.0f

typedef enum {
	WEUI_GFX_SCENEMGR_MSG_INPUT = 0,
	WEUI_GFX_SCENEMGR_MSG_GOTO,
	WEUI_GFX_SCENEMGR_MSG_CLEAR,
	WEUI_GFX_SCENEMGR_MSG_ACTION,
} WeUi_Gfx_SceneMgr_MsgType_t;

typedef struct {
	WeUi_Gfx_SceneMgr_MsgType_t Type;
	union {
		u8 InputRaw[WEUI_GFX_SCENEMGR_INPUT_MAX_BYTES];
		u32 SceneIdx;
		VoidFunc_t ActionFn;
		struct {
			void (*Fn)(void* pCtx);
			void* pCtx;
		} ClearDone;
	};
} WeUi_Gfx_SceneMgr_Msg_t;

static WeUi_Gfx_SceneMgrCfg_t WeUi_Gfx_SceneMgr_Cfg;

static WeUi_Gfx_SceneMgr_Msg_t WeUi_Gfx_SceneMgr_Ring[WEUI_GFX_SCENEMGR_QUEUE_DEPTH];
static u32 WeUi_Gfx_SceneMgr_RingHead;
static u32 WeUi_Gfx_SceneMgr_RingTail;
static u32 WeUi_Gfx_SceneMgr_RingCnt;

static u32 WeUi_Gfx_SceneMgr_NextFrameMs;
static bool WeUi_Gfx_SceneMgr_FrameScheduled;
static u32 WeUi_Gfx_SceneMgr_CurScene;
static bool WeUi_Gfx_SceneMgr_HasScene;

/* What GoTo() chose, and for which switch */
static WeUi_Gfx_SceneMgr_TransitionFn_t WeUi_Gfx_SceneMgr_PendingFn;
static u32 WeUi_Gfx_SceneMgr_PendingScene;

/* When the active scene's time is up - see cfg.TimeoutsMs */
static u32 WeUi_Gfx_SceneMgr_TimeoutAtMs;
static bool WeUi_Gfx_SceneMgr_TimeoutArmed;

static bool WeUi_Gfx_SceneMgr_RingPush(const WeUi_Gfx_SceneMgr_Msg_t* pcMsg) {
	WEUI_CRITICAL_DECL();
	bool pushed = false;

	WEUI_ENTER_CRITICAL();
	if (WeUi_Gfx_SceneMgr_RingCnt < WEUI_GFX_SCENEMGR_QUEUE_DEPTH) {
		WeUi_Gfx_SceneMgr_Ring[WeUi_Gfx_SceneMgr_RingTail] = *pcMsg;
		WeUi_Gfx_SceneMgr_RingTail =
			(WeUi_Gfx_SceneMgr_RingTail + 1) % WEUI_GFX_SCENEMGR_QUEUE_DEPTH;
		WeUi_Gfx_SceneMgr_RingCnt++;
		pushed = true;
	}
	WEUI_EXIT_CRITICAL();

	return pushed;
}

static bool WeUi_Gfx_SceneMgr_RingPop(WeUi_Gfx_SceneMgr_Msg_t* pMsg) {
	WEUI_CRITICAL_DECL();
	bool popped = false;

	WEUI_ENTER_CRITICAL();
	if (WeUi_Gfx_SceneMgr_RingCnt) {
		*pMsg = WeUi_Gfx_SceneMgr_Ring[WeUi_Gfx_SceneMgr_RingHead];
		WeUi_Gfx_SceneMgr_RingHead =
			(WeUi_Gfx_SceneMgr_RingHead + 1) % WEUI_GFX_SCENEMGR_QUEUE_DEPTH;
		WeUi_Gfx_SceneMgr_RingCnt--;
		popped = true;
	}
	WEUI_EXIT_CRITICAL();

	return popped;
}

static const WeUi_Gfx_SceneVtbl_t* WeUi_Gfx_SceneMgr_VtblGet(u32 idx) {
	if (!WeUi_Gfx_SceneMgr_Cfg.Scenes || idx >= WeUi_Gfx_SceneMgr_Cfg.SceneCount) {
		return NULL;
	}

	return WeUi_Gfx_SceneMgr_Cfg.Scenes[idx];
}

static void WeUi_Gfx_SceneMgr_DrawOverlay(void) {
	if (!WeUi_Gfx_SceneMgr_Cfg.Overlay)
		return;

	WeUi_Gfx_Point_t refPt = WeUi_Gfx_SceneMgr_Cfg.RefPt;
	WeUi_Gfx_SceneMgr_Cfg.Overlay(&refPt);
}

/* One frame of the active scene for a transition to compose: drawn as Poll() draws
 * it, but paced here and never pushed - the effect pushes what it composed */
static void WeUi_Gfx_SceneMgr_RenderIncoming(void) {
	const WeUi_Gfx_SceneVtbl_t* pcVtbl =
		WeUi_Gfx_SceneMgr_HasScene ? WeUi_Gfx_SceneMgr_VtblGet(WeUi_Gfx_SceneMgr_CurScene) : NULL;

	WeUi_Gfx_Adapter_ClearDrawingFrame();
	if (pcVtbl && pcVtbl->Render) {
		WeUi_Gfx_Point_t refPt = WeUi_Gfx_SceneMgr_Cfg.RefPt;
		pcVtbl->Render(&refPt);
		WeUi_Gfx_SceneMgr_DrawOverlay();
	}
	WeUi_Gfx_FpsCtrl_DrawAndKeepFPS_MainFreq();
}

/** @brief Draw the next frame at the first opportunity. */
static void WeUi_Gfx_SceneMgr_ScheduleNow(void) {
	WeUi_Gfx_SceneMgr_NextFrameMs	 = WeUi_Gfx_FpsCtrl_GetTickCnt();
	WeUi_Gfx_SceneMgr_FrameScheduled = true;
}

static void WeUi_Gfx_SceneMgr_ScheduleNext(float fps, u32 nowMs) {
	if (fps <= 0.0f) {
		WeUi_Gfx_SceneMgr_FrameScheduled = false;
		return;
	}

	/* The main frequency is the panel's overall ceiling and the debug knob a
	 * shell turns: lowering it slows scenes and transitions alike, which is what
	 * makes a transition inspectable. Nothing caps a scene from below - the
	 * frequency limits are about the pace of an animation's frames, while this
	 * is about how often a picture is worth redrawing, and an e-ink scene
	 * measures that in seconds */
	float mainFps = (float)WeUi_Gfx_FpsCtrl_GetMainFreq();
	if (mainFps > 0.0f && fps > mainFps) {
		fps = mainFps;
	}

	WeUi_Gfx_SceneMgr_NextFrameMs	 = nowMs + (u32)(WEUI_GFX_SCENEMGR_MS_IN_SEC / fps);
	WeUi_Gfx_SceneMgr_FrameScheduled = true;
}

/* Signed difference, so the tick counter wrapping does not strand a frame or a timeout */
static u32 WeUi_Gfx_SceneMgr_MsLeft(u32 atMs, u32 nowMs) {
	s32 leftMs = (s32)(atMs - nowMs);

	return (leftMs > 0) ? (u32)leftMs : 0;
}

static bool WeUi_Gfx_SceneMgr_FrameDue(u32 nowMs) {
	return WeUi_Gfx_SceneMgr_FrameScheduled &&
		   !WeUi_Gfx_SceneMgr_MsLeft(WeUi_Gfx_SceneMgr_NextFrameMs, nowMs);
}

static bool WeUi_Gfx_SceneMgr_TimeoutDue(u32 nowMs) {
	return WeUi_Gfx_SceneMgr_TimeoutArmed &&
		   !WeUi_Gfx_SceneMgr_MsLeft(WeUi_Gfx_SceneMgr_TimeoutAtMs, nowMs);
}

static u32 WeUi_Gfx_SceneMgr_ActiveOrSelf(void) {
	return WeUi_Gfx_SceneMgr_HasScene ? WeUi_Gfx_SceneMgr_CurScene : WEUI_GFX_SCENEMGR_SELF;
}

static void WeUi_Gfx_SceneMgr_ArmTimeout(void) {
	u32 ms = (WeUi_Gfx_SceneMgr_Cfg.TimeoutsMs && WeUi_Gfx_SceneMgr_HasScene)
				 ? WeUi_Gfx_SceneMgr_Cfg.TimeoutsMs[WeUi_Gfx_SceneMgr_CurScene]
				 : 0;

	WeUi_Gfx_SceneMgr_TimeoutArmed = (ms > 0);
	WeUi_Gfx_SceneMgr_TimeoutAtMs  = WeUi_Gfx_FpsCtrl_GetTickCnt() + ms;
}

/**
 * @brief Run the switch. @p reenter allows next == the active scene, which runs
 *        Exit() and Enter() again rather than doing nothing.
 */
static void WeUi_Gfx_SceneMgr_PlayInto(WeUi_Gfx_SceneMgr_TransitionFn_t transition,
									   WeUi_Gfx_SceneMgr_Cause_t cause);

static void WeUi_Gfx_SceneMgr_GotoScene(u32 next, bool reenter, WeUi_Gfx_SceneMgr_Cause_t cause) {
	/* Taken before anything can return: a transition belongs to the one switch it was
	 * chosen for, never to a later one */
	WeUi_Gfx_SceneMgr_TransitionFn_t transition =
		(WeUi_Gfx_SceneMgr_PendingScene == next) ? WeUi_Gfx_SceneMgr_PendingFn : NULL;
	WeUi_Gfx_SceneMgr_PendingFn = NULL;

	if (!reenter && WeUi_Gfx_SceneMgr_HasScene && next == WeUi_Gfx_SceneMgr_CurScene) {
		return;
	}

	const WeUi_Gfx_SceneVtbl_t* pcNext = WeUi_Gfx_SceneMgr_VtblGet(next);
	if (!pcNext) {
		return;
	}

	u32 from = WeUi_Gfx_SceneMgr_ActiveOrSelf();
	const WeUi_Gfx_SceneVtbl_t* pcCur =
		WeUi_Gfx_SceneMgr_HasScene ? WeUi_Gfx_SceneMgr_VtblGet(WeUi_Gfx_SceneMgr_CurScene) : NULL;

	if (pcCur && pcCur->Exit) {
		pcCur->Exit();
	}
	if (pcNext->FullClearOnEnter) {
		WeUi_Gfx_Adapter_ClearAllFrames();
	}

	WeUi_Gfx_SceneMgr_CurScene = next;
	WeUi_Gfx_SceneMgr_HasScene = true;

	if (pcNext->Enter) {
		pcNext->Enter();
	}

	WeUi_Gfx_SceneMgr_Trace((WeUi_Gfx_SceneMgr_Trace_t){
		.Kind		= WEUI_GFX_SCENEMGR_TRACE_SWITCH,
		.Cause		= cause,
		.Scene		= next,
		.From		= from,
		.Transition = transition,
	});
	WeUi_Gfx_SceneMgr_PlayInto(transition, cause);
	WeUi_Gfx_SceneMgr_ScheduleNow();
}

/** @brief Go where an OnInput or OnTimeout pointed */
static void WeUi_Gfx_SceneMgr_Follow(u32 next, WeUi_Gfx_SceneMgr_Cause_t cause,
									 const void* pcInput) {
	WeUi_Gfx_SceneMgr_Trace((WeUi_Gfx_SceneMgr_Trace_t){
		.Kind	 = WEUI_GFX_SCENEMGR_TRACE_ROUTED,
		.Cause	 = cause,
		.Scene	 = next,
		.From	 = WeUi_Gfx_SceneMgr_ActiveOrSelf(),
		.pcInput = pcInput,
	});

	if (next != WEUI_GFX_SCENEMGR_SELF) {
		/* A scene naming itself means "start me over", so an input that
		 * leads home is one case whether or not home is where it lands */
		WeUi_Gfx_SceneMgr_GotoScene(next, true, cause);
	}

	/* A GoTo() whose switch never came goes with the call that chose it */
	WeUi_Gfx_SceneMgr_PendingFn = NULL;
	WeUi_Gfx_SceneMgr_ScheduleNow();
}

static void WeUi_Gfx_SceneMgr_RouteInput(const void* pcInput) {
	const WeUi_Gfx_SceneVtbl_t* pcVtbl = WeUi_Gfx_SceneMgr_VtblGet(WeUi_Gfx_SceneMgr_CurScene);
	u32 (*onInput)(const void*) =
		(pcVtbl && pcVtbl->OnInput) ? pcVtbl->OnInput : WeUi_Gfx_SceneMgr_Cfg.OnInput;

	WeUi_Gfx_SceneMgr_Follow(onInput ? onInput(pcInput) : WEUI_GFX_SCENEMGR_SELF,
							 WEUI_GFX_SCENEMGR_CAUSE_INPUT, pcInput);
}

static void WeUi_Gfx_SceneMgr_RouteTimeout(void) {
	WeUi_Gfx_SceneMgr_TimeoutArmed = false;

	u32 (*onTimeout)(void) = WeUi_Gfx_SceneMgr_Cfg.OnTimeout;
	WeUi_Gfx_SceneMgr_Follow(onTimeout ? onTimeout() : WEUI_GFX_SCENEMGR_SELF,
							 WEUI_GFX_SCENEMGR_CAUSE_TIMEOUT, NULL);
}

static void WeUi_Gfx_SceneMgr_HandleMsg(const WeUi_Gfx_SceneMgr_Msg_t* pcMsg) {
	switch (pcMsg->Type) {
		case WEUI_GFX_SCENEMGR_MSG_GOTO:
			WeUi_Gfx_SceneMgr_GotoScene(pcMsg->SceneIdx, false, WEUI_GFX_SCENEMGR_CAUSE_REQUEST);
			break;

		case WEUI_GFX_SCENEMGR_MSG_CLEAR:
			WeUi_Gfx_SceneMgr_GotoScene(WeUi_Gfx_SceneMgr_Cfg.ClearScene, false,
										WEUI_GFX_SCENEMGR_CAUSE_CLEAR);
			if (pcMsg->ClearDone.Fn) {
				pcMsg->ClearDone.Fn(pcMsg->ClearDone.pCtx);
			}
			break;

		case WEUI_GFX_SCENEMGR_MSG_ACTION:
			if (pcMsg->ActionFn) {
				WeUi_Gfx_SceneMgr_Trace_t trace = {
					.Kind	= WEUI_GFX_SCENEMGR_TRACE_ACTION,
					.Scene	= WeUi_Gfx_SceneMgr_ActiveOrSelf(),
					.From	= WEUI_GFX_SCENEMGR_SELF,
					.Action = pcMsg->ActionFn,
				};
				WeUi_Gfx_SceneMgr_Trace(trace);
				pcMsg->ActionFn();
				trace.Kind = WEUI_GFX_SCENEMGR_TRACE_ACTION_DONE;
				WeUi_Gfx_SceneMgr_Trace(trace);
			}
			WeUi_Gfx_SceneMgr_ScheduleNow();
			break;

		case WEUI_GFX_SCENEMGR_MSG_INPUT:
		default:
			WeUi_Gfx_SceneMgr_RouteInput(pcMsg->InputRaw);
			break;
	}
}

void WeUi_Gfx_SceneMgr_Init(const WeUi_Gfx_SceneMgrCfg_t* pcCfg) {
	if (!pcCfg) {
		return;
	}

	WeUi_Gfx_SceneMgr_Cfg			 = *pcCfg;
	WeUi_Gfx_SceneMgr_RingHead		 = 0;
	WeUi_Gfx_SceneMgr_RingTail		 = 0;
	WeUi_Gfx_SceneMgr_RingCnt		 = 0;
	WeUi_Gfx_SceneMgr_HasScene		 = false;
	WeUi_Gfx_SceneMgr_FrameScheduled = false;
	WeUi_Gfx_SceneMgr_PendingFn		 = NULL;
	WeUi_Gfx_SceneMgr_TimeoutArmed	 = false;
}

u32 WeUi_Gfx_SceneMgr_Poll(void) {
	/* Not in Init(): the home scene's Enter() and the intro have to run once the host
	 * has the display up, not while it is still wiring its resources together */
	if (!WeUi_Gfx_SceneMgr_HasScene) {
		WeUi_Gfx_SceneMgr_GoTo(WeUi_Gfx_SceneMgr_Cfg.HomeScene, WeUi_Gfx_SceneMgr_Cfg.Intro);
		WeUi_Gfx_SceneMgr_GotoScene(WeUi_Gfx_SceneMgr_Cfg.HomeScene, false,
									WEUI_GFX_SCENEMGR_CAUSE_HOME);
	}

	/* Drained, not one per turn: a burst of messages would otherwise cost a
	 * frame each, and the host has already paid for being here */
	WeUi_Gfx_SceneMgr_Msg_t msg;
	while (WeUi_Gfx_SceneMgr_RingPop(&msg)) {
		WeUi_Gfx_SceneMgr_HandleMsg(&msg);
	}

	/* After the messages: a switch they caused has started its scene's time over */
	if (WeUi_Gfx_SceneMgr_TimeoutDue(WeUi_Gfx_FpsCtrl_GetTickCnt())) {
		WeUi_Gfx_SceneMgr_RouteTimeout();
	}

	const WeUi_Gfx_SceneVtbl_t* pcVtbl = WeUi_Gfx_SceneMgr_VtblGet(WeUi_Gfx_SceneMgr_CurScene);
	u32 nowMs						   = WeUi_Gfx_FpsCtrl_GetTickCnt();

	if (WeUi_Gfx_SceneMgr_FrameDue(nowMs)) {
		u32 frameStartMs = nowMs;

		if (pcVtbl && pcVtbl->Render) {
			if (pcVtbl->KeepFrame) {
				WeUi_Gfx_Adapter_CopyFrame_DisplayToDrawing();
			} else {
				WeUi_Gfx_Adapter_ClearDrawingFrame();
			}

			pcVtbl->Render(&WeUi_Gfx_SceneMgr_Cfg.RefPt);
			WeUi_Gfx_SceneMgr_DrawOverlay();
			WeUi_Gfx_Adapter_DrawFrame();
		}

		/* Drawing took time of its own, and the sleep below is measured from
		 * here rather than from before the frame */
		nowMs = WeUi_Gfx_FpsCtrl_GetTickCnt();

		WeUi_Gfx_SceneMgr_ScheduleNext(pcVtbl ? pcVtbl->Fps : 0.0f,
									   WEUI_GFX_SCENEMGR_PACE_FROM_FRAME_END ? nowMs
																			 : frameStartMs);
	}

	u32 sleepMs = WEUI_GFX_SCENEMGR_SLEEP_FOREVER;

	if (WeUi_Gfx_SceneMgr_FrameScheduled) {
		sleepMs = WeUi_Gfx_SceneMgr_MsLeft(WeUi_Gfx_SceneMgr_NextFrameMs, nowMs);
	}

	if (WeUi_Gfx_SceneMgr_TimeoutArmed) {
		sleepMs = GET_MIN(sleepMs, WeUi_Gfx_SceneMgr_MsLeft(WeUi_Gfx_SceneMgr_TimeoutAtMs, nowMs));
	}

	return sleepMs;
}

bool WeUi_Gfx_SceneMgr_PostInput(const void* pcInput, u32 size) {
	if (!pcInput || size > sizeof(((WeUi_Gfx_SceneMgr_Msg_t*)0)->InputRaw)) {
		return false;
	}

	WeUi_Gfx_SceneMgr_Msg_t msg = {.Type = WEUI_GFX_SCENEMGR_MSG_INPUT};
	memcpy(msg.InputRaw, pcInput, size);

	return WeUi_Gfx_SceneMgr_RingPush(&msg);
}

bool WeUi_Gfx_SceneMgr_RequestScene(u32 sceneIdx) {
	WeUi_Gfx_SceneMgr_Msg_t msg = {
		.Type	  = WEUI_GFX_SCENEMGR_MSG_GOTO,
		.SceneIdx = sceneIdx,
	};

	return WeUi_Gfx_SceneMgr_RingPush(&msg);
}

bool WeUi_Gfx_SceneMgr_PostAction(VoidFunc_t Fn) {
	if (!Fn) {
		return false;
	}

	WeUi_Gfx_SceneMgr_Msg_t msg = {
		.Type	  = WEUI_GFX_SCENEMGR_MSG_ACTION,
		.ActionFn = Fn,
	};

	return WeUi_Gfx_SceneMgr_RingPush(&msg);
}

bool WeUi_Gfx_SceneMgr_RequestClear(void (*OnDone)(void* pCtx), void* pCtx) {
	WeUi_Gfx_SceneMgr_Msg_t msg = {
		.Type = WEUI_GFX_SCENEMGR_MSG_CLEAR,
		.ClearDone =
			{
				.Fn	  = OnDone,
				.pCtx = pCtx,
			},
	};

	return WeUi_Gfx_SceneMgr_RingPush(&msg);
}

u32 WeUi_Gfx_SceneMgr_GoTo(u32 target, WeUi_Gfx_SceneMgr_TransitionFn_t transition) {
	WeUi_Gfx_SceneMgr_PendingFn	   = transition;
	WeUi_Gfx_SceneMgr_PendingScene = target;

	return target;
}

static void WeUi_Gfx_SceneMgr_PlayInto(WeUi_Gfx_SceneMgr_TransitionFn_t transition,
									   WeUi_Gfx_SceneMgr_Cause_t cause) {
	WeUi_Gfx_SceneMgr_Trace_t trace = {
		.Cause		= cause,
		.Scene		= WeUi_Gfx_SceneMgr_ActiveOrSelf(),
		.From		= WEUI_GFX_SCENEMGR_SELF,
		.Transition = transition,
	};

	if (transition) {
		trace.Kind = WEUI_GFX_SCENEMGR_TRACE_PLAY;
		WeUi_Gfx_SceneMgr_Trace(trace);
		transition(WeUi_Gfx_SceneMgr_RenderIncoming);
	}

	/* Started only now that it is on the panel: the transition is no part of a scene's time */
	WeUi_Gfx_SceneMgr_ArmTimeout();
	trace.Kind = WEUI_GFX_SCENEMGR_TRACE_SHOWN;
	WeUi_Gfx_SceneMgr_Trace(trace);
}

void WeUi_Gfx_SceneMgr_PlayIntoActive(WeUi_Gfx_SceneMgr_TransitionFn_t transition) {
	WeUi_Gfx_SceneMgr_PlayInto(transition, WEUI_GFX_SCENEMGR_CAUSE_HANDBACK);
}

WeUi_Gfx_Point_t WeUi_Gfx_SceneMgr_GetRefPt(void) {
	return WeUi_Gfx_SceneMgr_Cfg.RefPt;
}

u32 WeUi_Gfx_SceneMgr_GetCurScene(void) {
	return WeUi_Gfx_SceneMgr_ActiveOrSelf();
}
