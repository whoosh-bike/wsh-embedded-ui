#include "weui_gfx_effect_blocks.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

#define WEUI_GFX_EFFECT_BLOCK_W 6
#define WEUI_GFX_EFFECT_BLOCK_H 5

static void WeUi_Gfx_Effect_Row_Blocks(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	for (WeUi_Gfx_PtPos_t x = 0; x < width; x += WEUI_GFX_EFFECT_BLOCK_W) {
		WeUi_Gfx_PtPos_t n = GET_MIN(WEUI_GFX_EFFECT_BLOCK_W, width - x);
		float turn		   = WeUi_Gfx_Effect_Turn((u32)x / WEUI_GFX_EFFECT_BLOCK_W,
												  (u32)y / WEUI_GFX_EFFECT_BLOCK_H);
		if (turn >= pcCtx->Progress)
			WeUi_Gfx_Effect_RowOld(x, x, y, n);
		else
			WeUi_Gfx_Effect_RowNew(x, x, y, n);
	}
}

RET_STATE_t WeUi_Gfx_Effect_Blocks(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Blocks, NULL, frames, NULL, renderIncoming);
}
