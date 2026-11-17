#include "weui_gfx_effect_perlin.h"
#include "def_macro.h"
#include "mathlib_common.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_effect_runner.h"

static const WeUi_Gfx_Effect_PerlinCfg_t WeUi_Gfx_Effect_PerlinCfg_Def = {
	.ScaleX	   = 0.20f,
	.ScaleY	   = 0.40f,
	.Drift	   = 3.0f,
	.Gain	   = 1.40f,
	.RevealLow = 0.25f,
	.EdgeSoft  = 0.35f,
	.HueBase   = 270.0f,
	.HueSpan   = 120.0f,
	.HueSpin   = 200.0f,
};

const WeUi_Gfx_Effect_PerlinCfg_t* WeUi_Gfx_Effect_PerlinCfg_Default(void) {
	return &WeUi_Gfx_Effect_PerlinCfg_Def;
}

void WeUi_Gfx_Effect_Perlin_PaintField(const WeUi_Gfx_Effect_PerlinField_t* pcField,
									   const WeUi_Gfx_Effect_PerlinCfg_t* pcCfg) {
	if (!pcCfg)
		pcCfg = &WeUi_Gfx_Effect_PerlinCfg_Def;

	WeUi_Gfx_ObjSize_t width  = WeUi_Gfx_Adapter_GetWidth();
	WeUi_Gfx_ObjSize_t height = WeUi_Gfx_Adapter_GetHeight();

	for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)width; x++) {
		for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)height; y++) {
			/* perlin_3D truncates towards zero, so its coordinates must stay positive */
			float noise = perlin_3D((float)x * pcCfg->ScaleX + pcField->OffsetX,
									(float)y * pcCfg->ScaleY + pcField->OffsetY, pcField->Time);
			float value = CUT_NORM_FLOAT(SHIFT_AND_NORM(noise * pcCfg->Gain));

			WeUi_Gfx_ColorSchemeHsb_t hsbColor = {
				.Hue = WeUi_Color_HueIncDec(pcCfg->HueBase + pcCfg->HueSpan * value,
											pcField->HueShift),
				.Sat = WEUI_COLOR_HSB_MAX_SAT,
				.Bri = WEUI_COLOR_HSB_MAX_BRI *
					   CUT_NORM_FLOAT((value - pcField->Threshold) / pcCfg->EdgeSoft),
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

void WeUi_Gfx_Effect_Perlin_Paint(float progress, const WeUi_Gfx_Effect_PerlinCfg_t* pcCfg) {
	if (!pcCfg)
		pcCfg = &WeUi_Gfx_Effect_PerlinCfg_Def;

	/* Falls from above the field's ceiling to below its floor: pixels light up as
	 * the threshold passes their noise value, and none are left dark at the end */
	const WeUi_Gfx_Effect_PerlinField_t field = {
		.Time	   = progress * pcCfg->Drift,
		.Threshold = 1.0f - progress * (1.0f + pcCfg->RevealLow),
		.HueShift  = pcCfg->HueSpin * progress,
		.OffsetX   = 0.0f,
		.OffsetY   = 0.0f,
	};

	WeUi_Gfx_Effect_Perlin_PaintField(&field, pcCfg);
}

static void WeUi_Gfx_Effect_Perlin_Frame(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	WeUi_Gfx_Effect_Perlin_Paint(pcCtx->Progress, pcCtx->ParamsPtr);
}

RET_STATE_t WeUi_Gfx_Effect_Perlin_Play(u16 frames, const WeUi_Gfx_Effect_PerlinCfg_t* pcCfg) {
	return WeUi_Gfx_Effect_RunSolo(WeUi_Gfx_Effect_Perlin_Frame, frames, pcCfg);
}
