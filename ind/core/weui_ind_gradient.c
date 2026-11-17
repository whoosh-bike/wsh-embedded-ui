#include "weui_ind_gradient.h"
#include "weui_colors.h"
#include "weui_ind_gradient_data.h"

static WeUi_Ind_Grad_t IndGradMapList[WEUI_IND_GRAD_ENUM_SIZE] = {
#define X_ENTRY(name) {name, NUM_ELEMENTS(name)},
	WEUI_IND_GRADIENT_MAP_LIST
#undef X_ENTRY
};

WeUi_ColorSchemeHsb_t WeUi_Ind_Gradient_GetColor(WEUI_IND_GRAD_t grad, u32 idx) {
	if (grad >= WEUI_IND_GRAD_ENUM_SIZE || idx >= IndGradMapList[grad].Len) {
		PANIC();
		return WeUi_Color_GetEmptyHsb();
	}

	return IndGradMapList[grad].Ptr[idx];
}

u32 WeUi_Ind_Gradient_GetLen(WEUI_IND_GRAD_t grad) {
	if (grad >= WEUI_IND_GRAD_ENUM_SIZE) {
		PANIC();
		return 0;
	}

	return IndGradMapList[grad].Len;
}

u16 WeUi_Ind_Gradient_GetIdxByNorm(WEUI_IND_GRAD_t grad, float normIdx) {
	return (u16)(CUT_NORM_FLOAT(normIdx) * (float)(WeUi_Ind_Gradient_GetLen(grad) - 1));
}

WeUi_ColorSchemeHsb_t WeUi_Ind_Gradient_GetColorByNorm(WEUI_IND_GRAD_t grad, float normIdx) {
	return WeUi_Ind_Gradient_GetColor(grad, WeUi_Ind_Gradient_GetIdxByNorm(grad, normIdx));
}

u16 WeUi_Ind_Gradient_GetWrapIdxByCircle(WEUI_IND_GRAD_t grad, float circleIdx) {
	// 0→1, 1.2→0.8, 2.3→0.3, 3.7→0.7, ...
	u32 intPart	   = (u32)circleIdx;
	float fracPart = circleIdx - (float)intPart;
	float wrapped  = (intPart % 2 == 0) ? fracPart : 1.0f - fracPart;

	return WeUi_Ind_Gradient_GetIdxByNorm(grad, wrapped);
}

WeUi_ColorSchemeHsb_t WeUi_Ind_Gradient_GetWrapColorByCircle(WEUI_IND_GRAD_t grad,
															 float circleIdx) {
	return WeUi_Ind_Gradient_GetColor(grad, WeUi_Ind_Gradient_GetWrapIdxByCircle(grad, circleIdx));
}
