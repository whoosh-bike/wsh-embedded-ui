#include "weui_gfx_scenemgr_trace.h"

static void (*WeUi_Gfx_SceneMgr_TraceFn)(const WeUi_Gfx_SceneMgr_Trace_t* pcTrace);

void WeUi_Gfx_SceneMgr_SetTrace(void (*trace)(const WeUi_Gfx_SceneMgr_Trace_t* pcTrace)) {
	WeUi_Gfx_SceneMgr_TraceFn = trace;
}

void WeUi_Gfx_SceneMgr_Trace(WeUi_Gfx_SceneMgr_Trace_t trace) {
	if (WeUi_Gfx_SceneMgr_TraceFn)
		WeUi_Gfx_SceneMgr_TraceFn(&trace);
}
