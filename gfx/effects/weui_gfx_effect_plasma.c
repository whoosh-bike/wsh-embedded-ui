#include "weui_gfx_effect_plasma.h"
#include "def_macro.h"
#include "mathlib_common.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_effect_runner.h"

#define WEUI_GFX_EFFECT_PLASMA_WAVES_NUM 3.0f

static const WeUi_Gfx_Effect_PlasmaCfg_t WeUi_Gfx_Effect_PlasmaCfg_Def = {
	.WaveX	   = 0.42f,
	.WaveY	   = 0.62f,
	.WaveDiag  = 0.30f,
	.DriftDeg  = 540.0f,
	.DriftY	   = -1.30f,
	.DriftDiag = 0.70f,
	.Bloom	   = 0.30f,
	.HueBase   = 190.0f, /* cyan, so the span lands on blue - violet */
	.HueSpan   = 70.0f,
	.HueSpin   = 400.0f,
	.BriFloor  = 0.12f,
};

/* Sampled once a frame, read once a pixel: every pixel wants sin(x), sin(y) and
 * sin(x + y), each on one coordinate sum only, which turns 3 * W * H sines into
 * W + H + (W + H). Static rather than automatic - at panel size these run into
 * kilobytes, and the render task's stack is not where that belongs */
static float WeUi_Gfx_Effect_Plasma_WaveX[WEUI_DISPLAY_WIDTH];
static float WeUi_Gfx_Effect_Plasma_WaveY[WEUI_DISPLAY_HEIGHT];
static float WeUi_Gfx_Effect_Plasma_WaveDiag[WEUI_DISPLAY_WIDTH + WEUI_DISPLAY_HEIGHT];

const WeUi_Gfx_Effect_PlasmaCfg_t* WeUi_Gfx_Effect_PlasmaCfg_Default(void) {
	return &WeUi_Gfx_Effect_PlasmaCfg_Def;
}

void WeUi_Gfx_Effect_Plasma_Paint(float progress, const WeUi_Gfx_Effect_PlasmaCfg_t* pcCfg) {
	if (!pcCfg)
		pcCfg = &WeUi_Gfx_Effect_PlasmaCfg_Def;

	/* The wave buffers are sized from the config macros while the panel reports its
	 * own size: a config that has drifted from the driver clips the field instead
	 * of running off the end of them */
	WeUi_Gfx_ObjSize_t width  = WeUi_Gfx_Adapter_GetWidth();
	WeUi_Gfx_ObjSize_t height = WeUi_Gfx_Adapter_GetHeight();

	if (width > WEUI_DISPLAY_WIDTH)
		width = WEUI_DISPLAY_WIDTH;

	if (height > WEUI_DISPLAY_HEIGHT)
		height = WEUI_DISPLAY_HEIGHT;

	float phase = (float)D2R(pcCfg->DriftDeg) * progress;

	for (WeUi_Gfx_ObjSize_t x = 0; x < width; x++)
		WeUi_Gfx_Effect_Plasma_WaveX[x] = sinf((float)x * pcCfg->WaveX + phase);

	for (WeUi_Gfx_ObjSize_t y = 0; y < height; y++)
		WeUi_Gfx_Effect_Plasma_WaveY[y] = sinf((float)y * pcCfg->WaveY + phase * pcCfg->DriftY);

	for (WeUi_Gfx_ObjSize_t d = 0; d < width + height; d++)
		WeUi_Gfx_Effect_Plasma_WaveDiag[d] =
			sinf((float)d * pcCfg->WaveDiag + phase * pcCfg->DriftDiag);

	/* Only the opening frames are dimmed - the field arrives out of black */
	float bloom = CUT_NORM_FLOAT(progress / pcCfg->Bloom);

	for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)width; x++) {
		for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)height; y++) {
			/* Sum of the three waves, normalized to [0; 1] */
			float value =
				SHIFT_AND_NORM((WeUi_Gfx_Effect_Plasma_WaveX[x] + WeUi_Gfx_Effect_Plasma_WaveY[y] +
								WeUi_Gfx_Effect_Plasma_WaveDiag[x + y]) /
							   WEUI_GFX_EFFECT_PLASMA_WAVES_NUM);

			WeUi_Gfx_ColorSchemeHsb_t hsbColor = {
				.Hue = WeUi_Color_HueIncDec(pcCfg->HueBase + pcCfg->HueSpan * value,
											pcCfg->HueSpin * progress),
				.Sat = WEUI_COLOR_HSB_MAX_SAT,
				.Bri = WEUI_COLOR_HSB_MAX_BRI * bloom *
					   (pcCfg->BriFloor + (1.0f - pcCfg->BriFloor) * value),
			};

			WeUi_Gfx_ColorPoint_t cpt = {
				.Pt.X  = x,
				.Pt.Y  = y,
				.Color = WeUi_Gfx_Color_FromHsb(&hsbColor),
			};

			WeUi_Gfx_Point_Draw(&cpt);
		}
	}
}

static void WeUi_Gfx_Effect_Plasma_Frame(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	WeUi_Gfx_Effect_Plasma_Paint(pcCtx->Progress, pcCtx->ParamsPtr);
}

RET_STATE_t WeUi_Gfx_Effect_Plasma_Play(u16 frames, const WeUi_Gfx_Effect_PlasmaCfg_t* pcCfg) {
	return WeUi_Gfx_Effect_RunSolo(WeUi_Gfx_Effect_Plasma_Frame, frames, pcCfg);
}
