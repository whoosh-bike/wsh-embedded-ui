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

/* The vector scene whose last frame is on the panel, NULL under a drawn one; its last Build, for
 * the next one's dt; and whether the panel has shown anything else since, a switch or a
 * transition, so that the next frame goes whole */
static WeUi_Gfx_Scene_t* WeUi_Gfx_SceneMgr_ShownScene;
static u32 WeUi_Gfx_SceneMgr_BuiltMs;
static bool WeUi_Gfx_SceneMgr_HasBuilt;
static bool WeUi_Gfx_SceneMgr_Whole;

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

static bool WeUi_Gfx_SceneMgr_IsVector(const WeUi_Gfx_SceneVtbl_t* pcVtbl) {
	return pcVtbl && pcVtbl->Build;
}

/* The vector scene's next frame with the overlay's items over it; NULL when it was not built */
static WeUi_Gfx_Scene_t* WeUi_Gfx_SceneMgr_Build(const WeUi_Gfx_SceneVtbl_t* pcVtbl) {
	u32 nowMs = WeUi_Gfx_FpsCtrl_GetTickCnt();
	float dt  = WeUi_Gfx_SceneMgr_HasBuilt
					? (float)(nowMs - WeUi_Gfx_SceneMgr_BuiltMs) / WEUI_GFX_SCENEMGR_MS_IN_SEC
					: 0.0f;
	WeUi_Gfx_SceneMgr_BuiltMs  = nowMs;
	WeUi_Gfx_SceneMgr_HasBuilt = true;

	WeUi_Gfx_Scene_t* pScene = pcVtbl->Build(dt);
	if (pScene && WeUi_Gfx_SceneMgr_Cfg.OverlayItems) {
		WeUi_Gfx_SceneMgr_Cfg.OverlayItems(pScene);
	}
	return pScene;
}

/* What changed since the frame before onto the panel; all of it after anything else was there */
static void WeUi_Gfx_SceneMgr_PresentVector(const WeUi_Gfx_SceneVtbl_t* pcVtbl) {
	WeUi_Gfx_Scene_t* pScene = WeUi_Gfx_SceneMgr_Build(pcVtbl);
	if (!pScene) {
		return;
	}
	if (WeUi_Gfx_SceneMgr_Whole || pScene != WeUi_Gfx_SceneMgr_ShownScene) {
		WeUi_Gfx_Scene_Forget(pScene);
		WeUi_Gfx_SceneMgr_Whole = false;
	}

	WeUi_Gfx_ClipRect_t rects[WEUI_GFX_SCENEMGR_DAMAGE_RECTS];
	u8 num = WeUi_Gfx_Scene_Damage(pScene, rects, NUM_ELEMENTS(rects));
	RET_STATE_t rs;
	if (WeUi_Gfx_SceneMgr_Cfg.Present) {
		rs = WeUi_Gfx_SceneMgr_Cfg.Present(pScene, rects, num);
	} else {
		rs = WeUi_Gfx_Scene_Present(pScene, rects, num);
		WeUi_Gfx_Adapter_DrawFrame();
	}

	/* What did not make it is drawn again with the rest of the next frame */
	WeUi_Gfx_SceneMgr_ShownScene = pScene;
	WeUi_Gfx_SceneMgr_Whole		 = (rs != RET_STATE_SUCCESS);
}

/* One frame of the active scene for a transition to compose: drawn as Poll() draws
 * it, but paced here and never pushed - the effect pushes what it composed */
static void WeUi_Gfx_SceneMgr_RenderIncoming(void) {
	const WeUi_Gfx_SceneVtbl_t* pcVtbl =
		WeUi_Gfx_SceneMgr_HasScene ? WeUi_Gfx_SceneMgr_VtblGet(WeUi_Gfx_SceneMgr_CurScene) : NULL;

	if (WeUi_Gfx_SceneMgr_IsVector(pcVtbl)) {
		WeUi_Gfx_Scene_t* pScene = WeUi_Gfx_SceneMgr_Build(pcVtbl);
		if (pScene &&
			WeUi_Gfx_Scene_DrawToFrame(pScene, WEUI_GFX_FRAME_DRAWING) == RET_STATE_SUCCESS) {
			WeUi_Gfx_SceneMgr_ShownScene = pScene;
		}
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS_MainFreq();
		return;
	}

	WeUi_Gfx_SceneMgr_ShownScene = NULL;
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
	WeUi_Gfx_SceneMgr_Whole	   = true;

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
				/* It may have drawn on the panel without handing it back */
				WeUi_Gfx_SceneMgr_Whole = true;
				trace.Kind				= WEUI_GFX_SCENEMGR_TRACE_ACTION_DONE;
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
	WeUi_Gfx_SceneMgr_ShownScene	 = NULL;
	WeUi_Gfx_SceneMgr_HasBuilt		 = false;
	WeUi_Gfx_SceneMgr_Whole			 = true;
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

		if (WeUi_Gfx_SceneMgr_IsVector(pcVtbl)) {
			WeUi_Gfx_SceneMgr_PresentVector(pcVtbl);
		} else if (pcVtbl && pcVtbl->Render) {
			WeUi_Gfx_SceneMgr_ShownScene = NULL;
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

/* Into or out of a vector scene the transition needs frames, and with bands the picture on the
 * panel is in none of them: drawn there first, from the vector scene that is on it */
static void WeUi_Gfx_SceneMgr_PlayTransition(WeUi_Gfx_SceneMgr_TransitionFn_t transition,
											 WeUi_Gfx_SceneMgr_Trace_t trace) {
	const WeUi_Gfx_SceneVtbl_t* pcVtbl = WeUi_Gfx_SceneMgr_VtblGet(WeUi_Gfx_SceneMgr_CurScene);
	bool vector = WeUi_Gfx_SceneMgr_IsVector(pcVtbl) || WeUi_Gfx_SceneMgr_ShownScene;
	bool framed = vector && WeUi_Gfx_SceneMgr_Cfg.FramesOpen;

	if (framed && !WeUi_Gfx_SceneMgr_Cfg.FramesOpen()) {
		return;
	}
#if WEUI_DISPLAY_BANDS
	if (WeUi_Gfx_SceneMgr_ShownScene) {
		WeUi_Gfx_Scene_DrawToFrame(WeUi_Gfx_SceneMgr_ShownScene, WEUI_GFX_FRAME_DISPLAY);
	}
#endif /* WEUI_DISPLAY_BANDS */

	trace.Kind = WEUI_GFX_SCENEMGR_TRACE_PLAY;
	WeUi_Gfx_SceneMgr_Trace(trace);
	transition(WeUi_Gfx_SceneMgr_RenderIncoming);

	if (framed && WeUi_Gfx_SceneMgr_Cfg.FramesClose) {
		WeUi_Gfx_SceneMgr_Cfg.FramesClose();
	}
	/* Whatever its last frame left on the panel, the scene's next one redraws all of it */
	WeUi_Gfx_SceneMgr_Whole = true;
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
		WeUi_Gfx_SceneMgr_PlayTransition(transition, trace);
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
