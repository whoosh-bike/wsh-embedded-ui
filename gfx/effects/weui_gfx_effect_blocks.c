#include "weui_gfx_effect_blocks.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

#define WEUI_GFX_EFFECT_BLOCK_W 6
#define WEUI_GFX_EFFECT_BLOCK_H 5

static void WeUi_Gfx_Effect_Compose_Blocks(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)pcCtx->Height; y++) {
		for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)pcCtx->Width; x++) {
			float turn = WeUi_Gfx_Effect_Turn((u32)x / WEUI_GFX_EFFECT_BLOCK_W,
											  (u32)y / WEUI_GFX_EFFECT_BLOCK_H);
			if (turn >= pcCtx->Progress)
				WeUi_Gfx_Effect_PutOld(x, y, x, y);
		}
	}
}

RET_STATE_t WeUi_Gfx_Effect_Blocks(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Blocks, frames, NULL,
										 renderIncoming);
}
