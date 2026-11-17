#include "weui_gfx_fractal_tree.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"

#define WEUI_GFX_FRACTAL_TREE_FULL_TURN 360.0f

static WeUi_Gfx_Color_t WeUi_Gfx_FractalTree_LevelColor(const WeUi_Gfx_FractalTreeCfg_t* pcCfg,
														u32 level) {
	float toTips = (pcCfg->Depth > 1) ? (float)level / (float)(pcCfg->Depth - 1) : 0.0f;

	float hue = pcCfg->Trunk.Hue + (pcCfg->Tips.Hue - pcCfg->Trunk.Hue) * toTips;
	hue		  = fmodf(hue, WEUI_GFX_FRACTAL_TREE_FULL_TURN);
	if (hue < 0.0f)
		hue += WEUI_GFX_FRACTAL_TREE_FULL_TURN;

	WeUi_Gfx_ColorSchemeHsb_t hsb = {
		.Hue = (hue > WEUI_COLOR_HSB_MAX_HUE) ? WEUI_COLOR_HSB_MAX_HUE : hue,
		.Sat = pcCfg->Trunk.Sat + (pcCfg->Tips.Sat - pcCfg->Trunk.Sat) * toTips,
		.Bri = pcCfg->Trunk.Bri + (pcCfg->Tips.Bri - pcCfg->Trunk.Bri) * toTips,
	};

	return WeUi_Gfx_Color_FromHsb(&hsb);
}

static void WeUi_Gfx_FractalTree_Branch(const WeUi_Gfx_FractalTreeCfg_t* pcCfg, float x, float y,
										float len, float angle, u32 level) {
	float rad  = D2R(angle);
	float endX = x + len * cosf(rad);
	float endY = y - len * sinf(rad);

	WeUi_Gfx_Line_t line = {
		.Base.PtOne.X = (WeUi_Gfx_PtPos_t)roundf(x),
		.Base.PtOne.Y = (WeUi_Gfx_PtPos_t)roundf(y),
		.Base.PtTwo.X = (WeUi_Gfx_PtPos_t)roundf(endX),
		.Base.PtTwo.Y = (WeUi_Gfx_PtPos_t)roundf(endY),
		.Base.Color	  = WeUi_Gfx_FractalTree_LevelColor(pcCfg, level),
		.Hatching	  = 1.0f,
	};
	WeUi_Gfx_Line_Draw(&line);

	if (level + 1 >= pcCfg->Depth)
		return;

	len *= pcCfg->Shrink;
	WeUi_Gfx_FractalTree_Branch(pcCfg, endX, endY, len, angle + pcCfg->SpreadCcw + pcCfg->Sway,
								level + 1);
	WeUi_Gfx_FractalTree_Branch(pcCfg, endX, endY, len, angle - pcCfg->SpreadCw + pcCfg->Sway,
								level + 1);
}

void WeUi_Gfx_FractalTree_Draw(const WeUi_Gfx_FractalTreeCfg_t* pcCfg) {
	if (pcCfg->Depth == 0)
		return;

	WeUi_Gfx_FractalTree_Branch(pcCfg, pcCfg->RootX, pcCfg->RootY, pcCfg->Length, pcCfg->Angle, 0);
}
