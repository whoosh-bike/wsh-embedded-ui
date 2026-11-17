#ifndef __WEUI_IND_REMAP_H
#define __WEUI_IND_REMAP_H

#include "weui_ind_types.h"

/** @brief Set the active LED remap scheme. pScheme must point to an array of physical LED indices
 *         with at least as many entries as the logical pixel count used by widgets. */
void WeUi_Ind_LedRemap_SetScheme(const u16* pScheme);
void WeUi_Ind_LedRemap_SetPixel(u16 pixIdx, WeUi_ColorSchemeHsb_t pixColor);
WeUi_ColorSchemeHsb_t WeUi_Ind_LedRemap_GetPixel(u16 pixIdx);

#endif /* __WEUI_IND_REMAP_H */
