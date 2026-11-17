#include <emscripten.h>
#include <stdarg.h>
#include <stdio.h>

#include "display_m.h"
#include "lab_api.h"
#include "log.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_effects.h"
#include "weui_gfx_fps_control.h"
#include "weui_gfx_scenemgr_trace.h"
#include "weui_gfx_text.h"

/**
 * The wasm side of the lab. The UI starts a fresh module for every run, so a run
 * begins from the same state and replays bit for bit; frames go out as they are pushed.
 */

#define LAB_FRAME_FLAG_EFFECT (1u << 0)
#define LAB_LOG_LINE_MAX	  512

typedef struct {
	const char* Name;
	VoidFunc_t Fn;
} Lab_Action_t;

typedef struct {
	const char* Name;
	void (*Fn)(u32 arg);
	u32 Arg;
	const char* Group;
} Lab_Input_t;

static Lab_Action_t Actions[LAB_ACTIONS_MAX];
static u32 ActionsNum;
static Lab_Input_t Inputs[LAB_INPUTS_MAX];
static u32 InputsNum;
static u32 (*Poll)(void);
static const char* Group;
static u32 GroupsNum;
static u32 ScenesNum;
static const char* (*SceneName)(u32 scene);
static const char* (*InputName)(const void* pcInput);

EM_JS(void, Lab_Js_Frame, (const u32* pcFrame, u32 idx, u32 timeMs, u32 flags),
	  { Module.onFrame(pcFrame, idx, timeMs, flags); });

EM_JS(void, Lab_Js_Panic, (const char* pcFile, int line),
	  { Module.onPanic(UTF8ToString(pcFile), line); });

/* A function pointer is its index in the wasm table: the UI names it from the module */
EM_JS(void, Lab_Js_Trace,
	  (int kind, int cause, u32 scene, u32 from, u32 fn, const char* pcInput, u32 timeMs),
	  { Module.onTrace(kind, cause, scene, from, fn, UTF8ToString(pcInput), timeMs); });

EM_JS(void, Lab_Js_Log, (int lvl, const char* pcText, u32 timeMs),
	  { Module.onLog(lvl, UTF8ToString(pcText), timeMs); });

void Sim_Log(LOG_LVL_t lvl, const char* pcFmt, ...) {
	char line[LAB_LOG_LINE_MAX];
	va_list args;
	va_start(args, pcFmt);
	int len = vsnprintf(line, sizeof(line), pcFmt, args);
	va_end(args);

	/* The firmware's NL variants end in "\r\n"; a console line has its own break */
	len = GET_MIN(len, (int)sizeof(line) - 1);
	while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
		line[--len] = '\0';
	Lab_Js_Log(lvl, line, Sim_TickMs());
}

void Sim_Panic(const char* pcFile, int line) {
	Lab_Js_Panic(pcFile, line);
}

static void Lab_OnTrace(const WeUi_Gfx_SceneMgr_Trace_t* pcTrace) {
	const char* pcInput = (pcTrace->pcInput && InputName) ? InputName(pcTrace->pcInput) : "";
	u32 fn =
		pcTrace->Action ? (u32)(uintptr_t)pcTrace->Action : (u32)(uintptr_t)pcTrace->Transition;

	Lab_Js_Trace(pcTrace->Kind, pcTrace->Cause, pcTrace->Scene, pcTrace->From, fn,
				 pcInput ? pcInput : "", Sim_TickMs());
}

static void Lab_OnFrame(const u32* pcFrame, u32 idx) {
	u32 flags = WeUi_Gfx_Effect_IsPlaying() ? LAB_FRAME_FLAG_EFFECT : 0;
	Lab_Js_Frame(pcFrame, idx, Sim_TickMs(), flags);
}

void Lab_AddAction(const char* pcName, VoidFunc_t fn) {
	if (ActionsNum >= LAB_ACTIONS_MAX || !pcName || !fn) {
		PANIC();
		return;
	}
	Actions[ActionsNum++] = (Lab_Action_t){.Name = pcName, .Fn = fn};
}

void Lab_SetPoll(u32 (*poll)(void)) {
	Poll = poll;
}

void Lab_AddInput(const char* pcName, void (*fn)(u32 arg), u32 arg) {
	if (InputsNum >= LAB_INPUTS_MAX || !pcName || !fn) {
		PANIC();
		return;
	}
	Inputs[InputsNum++] = (Lab_Input_t){
		.Name  = pcName,
		.Fn	   = fn,
		.Arg   = arg,
		.Group = Group,
	};
}

void Lab_AddGroup(const char* pcName) {
	if (GroupsNum >= LAB_GROUPS_MAX || !pcName) {
		PANIC();
		return;
	}
	GroupsNum++;
	Group = pcName;
}

void Lab_SetScenes(u32 count, const char* (*name)(u32 scene)) {
	ScenesNum = count;
	SceneName = name;
}

void Lab_SetInputName(const char* (*name)(const void* pcInput)) {
	InputName = name;
}

/** @return false when the adapter refuses the panel - larger than the build's maximum */
EMSCRIPTEN_KEEPALIVE bool Lab_Start(u16 w, u16 h) {
	DisplaySim_Init(w, h);
	srand(1);

	if (WeUi_Gfx_Adapter_Init() != RET_STATE_SUCCESS)
		return false;

	WeUi_Gfx_Text_Init();
	WeUi_Gfx_FpsCtrl_AttachDrawCallback(WeUi_Gfx_Adapter_DrawFrame);
	WeUi_Gfx_FpsCtrl_SetMainFreqToDef();
	WeUi_Gfx_Adapter_ClearAllFrames();
	DisplaySim_SetFrameHook(Lab_OnFrame);
	WeUi_Gfx_SceneMgr_SetTrace(Lab_OnTrace);

	ActionsNum = 0;
	InputsNum  = 0;
	Poll	   = NULL;
	Group	   = NULL;
	GroupsNum  = 0;
	ScenesNum  = 0;
	SceneName  = NULL;
	InputName  = NULL;
	Lab_Target_Init();
	return true;
}

EMSCRIPTEN_KEEPALIVE u32 Lab_ActionsNum(void) {
	return ActionsNum;
}

EMSCRIPTEN_KEEPALIVE const char* Lab_ActionName(u32 idx) {
	return (idx < ActionsNum) ? Actions[idx].Name : "";
}

EMSCRIPTEN_KEEPALIVE void Lab_RunAction(u32 idx) {
	if (idx < ActionsNum)
		Actions[idx].Fn();
}

EMSCRIPTEN_KEEPALIVE bool Lab_IsLive(void) {
	return Poll != NULL;
}

EMSCRIPTEN_KEEPALIVE u32 Lab_InputsNum(void) {
	return InputsNum;
}

EMSCRIPTEN_KEEPALIVE const char* Lab_InputName(u32 idx) {
	return (idx < InputsNum) ? Inputs[idx].Name : "";
}

/** @return "" for an input added before any group */
EMSCRIPTEN_KEEPALIVE const char* Lab_InputGroup(u32 idx) {
	return (idx < InputsNum && Inputs[idx].Group) ? Inputs[idx].Group : "";
}

EMSCRIPTEN_KEEPALIVE u32 Lab_ScenesNum(void) {
	return ScenesNum;
}

EMSCRIPTEN_KEEPALIVE const char* Lab_SceneName(u32 idx) {
	const char* pcName = (idx < ScenesNum && SceneName) ? SceneName(idx) : NULL;
	return pcName ? pcName : "";
}

EMSCRIPTEN_KEEPALIVE void Lab_Input(u32 idx) {
	if (idx < InputsNum)
		Inputs[idx].Fn(Inputs[idx].Arg);
}

/**
 * @brief Run a live target up to virtual time untilMs, as its host loop would: poll,
 *        then sleep what it asked for. A poll may take time itself - an effect plays
 *        inside it - and a sleep is cut at untilMs, the next call polls early.
 */
EMSCRIPTEN_KEEPALIVE void Lab_Advance(u32 untilMs) {
	if (!Poll)
		return;

	while ((s32)(untilMs - Sim_TickMs()) > 0) {
		u32 sleepMs = Poll();
		s32 leftMs	= (s32)(untilMs - Sim_TickMs());
		if (leftMs <= 0)
			break;
		Sim_DelayMs(GET_MIN(GET_MAX(sleepMs, 1u), (u32)leftMs));
	}
}

EMSCRIPTEN_KEEPALIVE void Lab_SetBright(float scale) {
	DisplaySim_SetBrightScale(scale);
}

/** @brief For a printf, which reaches the UI through stdout with no time of its own */
EMSCRIPTEN_KEEPALIVE u32 Lab_TimeMs(void) {
	return Sim_TickMs();
}

EMSCRIPTEN_KEEPALIVE u32 Lab_MaxWidth(void) {
	return WEUI_DISPLAY_WIDTH;
}

EMSCRIPTEN_KEEPALIVE u32 Lab_MaxHeight(void) {
	return WEUI_DISPLAY_HEIGHT;
}

/* A gray panel keeps its luma in the low byte alone */
EMSCRIPTEN_KEEPALIVE int Lab_IsGray(void) {
#if defined(WEUI_DISPLAY_GRAY)
	return 1;
#else
	return 0;
#endif
}
