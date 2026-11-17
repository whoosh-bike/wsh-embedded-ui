#include "weui_ind_effects.h"
#include "log.h"
#include "weui_colors.h"
#include "weui_ind_adapter.h"
#include "weui_ind_fps_control.h"
#include "weui_ind_widget.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT LOG
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

void WeUi_Ind_Effect_DistortAndFlash(const WeUi_Ind_VirtMap_t* pVmap) {
	WeUi_Ind_FpsCtrl_SetMainFreq(WEUI_IND_FPS_CTRL_DEF_RENDER_FREQ);
	WeUi_Ind_Adapter_FillDrawingFrame(WeUi_Color_GetEmptyHsb());

	for (u32 i = 0; i < 50; i++) {
		for (u32 col = WEUI_COLOR_RED; col < WEUI_COLOR_ENUM_SIZE; col += 4) {
			WeUi_ColorSchemeHsb_t color = WeUi_Color_GetFullHsb(WEUI_COLOR_GET_HUE(col));
			WeUi_Ind_Widget_FillColor(color, pVmap);
			u32 freq = (u32)exp_curve(i, 0, 50, 30, 80, -0.1);
			WeUi_Ind_FpsCtrl_DrawAndKeepFPS(freq);
		}
	}

	WeUi_ColorSchemeHsb_t color = {
		.Sat = WEUI_COLOR_HSB_MIN_SAT,
		.Bri = 2,
	};
	WeUi_Ind_Widget_FillColor(color, pVmap);
	for (u32 i = 0; i < 70; i++) {
		WeUi_Ind_Widget_FillColor(color, pVmap);
		color.Bri++;
		WeUi_Ind_FpsCtrl_DrawAndKeepFPS_MainFreq();
	}
}

void WeUi_Ind_Effect_Perlin1D(u32 showCycles, WEUI_IND_GRAD_t grad, float speedScale,
							  float lenScale, const WeUi_Ind_VirtMap_t* pVmap) {
	WeUi_Ind_FpsCtrl_SetMainFreq(WEUI_IND_FPS_CTRL_DEF_RENDER_FREQ);
	WeUi_Ind_Adapter_FillDrawingFrame(WeUi_Color_GetEmptyHsb());

	for (u32 locCycle = 0; locCycle < showCycles; locCycle++) {
		WeUi_Ind_Widget_Perlin1D_Grad(WeUi_Ind_FpsCtrl_GetRenderCycle(), speedScale, lenScale, 3.0f,
									  grad, 2.0f, false, pVmap);

		WeUi_Ind_FpsCtrl_DrawAndKeepFPS_MainFreq();
	}
}

void WeUi_Ind_Effect_Perlin3D(u32 showCycles, WEUI_IND_GRAD_t grad, float speedScale,
							  float lenScale, float gradScale, const WeUi_Ind_VirtMap_t* pVmap) {
	WeUi_Ind_FpsCtrl_SetMainFreq(WEUI_IND_FPS_CTRL_DEF_RENDER_FREQ);
	WeUi_Ind_Adapter_FillDrawingFrame(WeUi_Color_GetEmptyHsb());

	for (u32 locCycle = 0; locCycle < showCycles; locCycle++) {
		WeUi_Ind_Widget_Perlin3D_Grad(WeUi_Ind_FpsCtrl_GetRenderCycle(), speedScale, lenScale,
									  gradScale, grad, 2.0f, -1.0f, pVmap);

		WeUi_Ind_FpsCtrl_DrawAndKeepFPS_MainFreq();
	}
}
