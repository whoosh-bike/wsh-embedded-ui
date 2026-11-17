#include "weui_gfx_effect_dissolve.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* A type width, not tuning: the threshold has to reach the top of the u8 each pixel's
 * noise is cut to, or the last pixels never turn over */
#define WEUI_GFX_EFFECT_NOISE_MAX UINT8_MAX

/**
 * @brief A salt spanning the full u32 the hash avalanches over: two transitions in a
 *        row do not dissolve alike.
 *
 * Two draws, not one: WeUi_Rand_Between works in s32 and cannot reach the sign
 * bit, so a single draw would leave the top of the range unreachable.
 */
static u32 WeUi_Gfx_Effect_SaltRand(void) {
	return ((u32)WeUi_Rand_Between(0, UINT16_MAX) << 16) | (u32)WeUi_Rand_Between(0, UINT16_MAX);
}

static void WeUi_Gfx_Effect_Compose_Dissolve(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	u32 salt	 = *(const u32*)pcCtx->ParamsPtr;
	u8 threshold = (u8)((u32)pcCtx->Frame * WEUI_GFX_EFFECT_NOISE_MAX / pcCtx->Frames);

	for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)pcCtx->Height; y++) {
		for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)pcCtx->Width; x++) {
			/* A shuffled index array would cost a byte per pixel and cap the panel at
			 * 256 px: the noise is a hash, stable per pixel and salt. Pixels whose turn
			 * has not come go back to the old picture; the last frame tops the
			 * threshold out and none survive */
			if ((u8)WeUi_Rand_Hash((u32)x, (u32)y, salt) <= threshold)
				continue;

			WeUi_Gfx_Effect_PutOld(x, y, x, y);
		}
	}
}

RET_STATE_t WeUi_Gfx_Effect_Dissolve(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	u32 salt = WeUi_Gfx_Effect_SaltRand();

	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Dissolve, frames, &salt,
										 renderIncoming);
}
