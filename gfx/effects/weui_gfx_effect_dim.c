#include "weui_gfx_effect_dim.h"
#include "weui_gfx_base.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* Eased rather than linear, so neither picture is seen braking into the dark */
static void WeUi_Gfx_Effect_Compose_Dim(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	float half = 2.0f * pcCtx->Progress;

	if (half >= 1.0f) {
		WeUi_Gfx_Frame_Fade(WeUi_Gfx_Effect_Smooth(half - 1.0f));
		return;
	}

	/* First half: the incoming frame is drawn but not shown yet - the old one goes
	 * back over all of it, dimmed */
	WeUi_Gfx_Effect_PutOldFrame(pcCtx);
	WeUi_Gfx_Frame_Fade(WeUi_Gfx_Effect_Smooth(1.0f - half));
}

RET_STATE_t WeUi_Gfx_Effect_Dim(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Dim, frames, NULL, renderIncoming);
}
