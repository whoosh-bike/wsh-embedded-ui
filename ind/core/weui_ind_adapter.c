#include "weui_ind_adapter.h"
#include "led_strip_m.h"

u16 WeUi_Ind_Adapter_GetLedsNum(void) {
	return LedStripM_GetLedsNum();
}

void WeUi_Ind_Adapter_SetPixel(u16 pixIdx, WeUi_ColorSchemeHsb_t pixColor) {
	LedStripM_SetPixel(pixIdx, pixColor);
}

WeUi_ColorSchemeHsb_t WeUi_Ind_Adapter_GetPixel(u16 pixIdx) {
	return LedStripM_GetPixel(pixIdx);
}

void WeUi_Ind_Adapter_FillDrawingFrame(WeUi_ColorSchemeHsb_t color) {
	LedStripM_FillDrawingFrame(color);
}

void WeUi_Ind_Adapter_DrawFrame(void) {
	LedStripM_DrawFrame();
}
