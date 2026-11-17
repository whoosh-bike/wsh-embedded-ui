#include "weui_ind_remap.h"
#include "weui_colors.h"
#include "weui_ind_adapter.h"

static const u16* WeUi_Ind_CurrLedRemapScheme = NULL;

void WeUi_Ind_LedRemap_SetScheme(const u16* pScheme) {
	WeUi_Ind_CurrLedRemapScheme = pScheme;
}

void WeUi_Ind_LedRemap_SetPixel(u16 pixIdx, WeUi_ColorSchemeHsb_t pixColor) {
	if (!WeUi_Ind_CurrLedRemapScheme)
		return;

	u16 newPixIdx = WeUi_Ind_CurrLedRemapScheme[pixIdx];
	if (newPixIdx < WeUi_Ind_Adapter_GetLedsNum())
		WeUi_Ind_Adapter_SetPixel(newPixIdx, pixColor);
}

WeUi_ColorSchemeHsb_t WeUi_Ind_LedRemap_GetPixel(u16 pixIdx) {
	if (!WeUi_Ind_CurrLedRemapScheme)
		return WeUi_Color_GetEmptyHsb();

	u16 newPixIdx = WeUi_Ind_CurrLedRemapScheme[pixIdx];
	if (newPixIdx < WeUi_Ind_Adapter_GetLedsNum())
		return WeUi_Ind_Adapter_GetPixel(newPixIdx);

	return WeUi_Color_GetEmptyHsb();
}
