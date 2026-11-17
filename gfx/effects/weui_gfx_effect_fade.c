#include "weui_gfx_effect_fade.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

static WeUi_Gfx_ColorChannel_t WeUi_Gfx_Effect_FadeMix(WeUi_Gfx_ColorChannel_t from,
													   WeUi_Gfx_ColorChannel_t to, float mix) {
	return (WeUi_Gfx_ColorChannel_t)((float)from + ((float)to - (float)from) * mix + 0.5f);
}

static void WeUi_Gfx_Effect_Compose_Fade(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	float mix = WeUi_Gfx_Effect_Smooth(pcCtx->Progress);

	for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)pcCtx->Height; y++) {
		for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)pcCtx->Width; x++) {
			WeUi_Gfx_Point_t pt = {.X = x, .Y = y};
			WeUi_Gfx_Color_t old =
				WeUi_Gfx_Transform_Bytes2Color(WeUi_Gfx_Adapter_GetPixelRaw_Aux(&pt));
			WeUi_Gfx_Color_t next =
				WeUi_Gfx_Transform_Bytes2Color(WeUi_Gfx_Adapter_GetPixelRaw(&pt));

			next.R = WeUi_Gfx_Effect_FadeMix(old.R, next.R, mix);
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
			next.G = WeUi_Gfx_Effect_FadeMix(old.G, next.G, mix);
			next.B = WeUi_Gfx_Effect_FadeMix(old.B, next.B, mix);
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

			WeUi_Gfx_Adapter_SetPixelRaw(&pt, WeUi_Gfx_Transform_Color2Bytes(next));
		}
	}
}

RET_STATE_t WeUi_Gfx_Effect_Fade(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Fade, frames, NULL,
										 renderIncoming);
}
