#include "weui_gfx_effect_interlace.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* Even rows come in from the left, odd ones from the right: each shifted copy walks
 * away from the side it reads from, so the incoming row is read before it is written */
static void WeUi_Gfx_Effect_Compose_Interlace(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	WeUi_Gfx_PtPos_t rest =
		(WeUi_Gfx_PtPos_t)((float)width * (1.0f - WeUi_Gfx_Effect_Smooth(pcCtx->Progress)) + 0.5f);

	for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)pcCtx->Height; y++) {
		bool fromLeft = (y % 2) == 0;

		for (WeUi_Gfx_PtPos_t i = 0; i < width; i++) {
			WeUi_Gfx_PtPos_t x	= fromLeft ? i : width - 1 - i;
			WeUi_Gfx_PtPos_t sx = fromLeft ? x + rest : x - rest;

			if (sx < 0 || sx >= width) {
				WeUi_Gfx_Effect_PutOld(x, y, x, y);
				continue;
			}

			WeUi_Gfx_Point_t dst = {.X = x, .Y = y};
			WeUi_Gfx_Point_t src = {.X = sx, .Y = y};
			WeUi_Gfx_Adapter_SetPixelRaw(&dst, WeUi_Gfx_Adapter_GetPixelRaw(&src));
		}
	}
}

RET_STATE_t WeUi_Gfx_Effect_Interlace(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Interlace, frames, NULL,
										 renderIncoming);
}
