#ifndef __WEUI_IND_ADAPTER_H
#define __WEUI_IND_ADAPTER_H

#include "weui_common_types.h"

/** @brief Return the number of LEDs in the strip */
u16 WeUi_Ind_Adapter_GetLedsNum(void);

/** @brief Write pixel color at physical index */
void WeUi_Ind_Adapter_SetPixel(u16 pixIdx, WeUi_ColorSchemeHsb_t pixColor);

/** @brief Read pixel color at physical index */
WeUi_ColorSchemeHsb_t WeUi_Ind_Adapter_GetPixel(u16 pixIdx);

/** @brief Fill the drawing frame with the given color */
void WeUi_Ind_Adapter_FillDrawingFrame(WeUi_ColorSchemeHsb_t color);

/** @brief Push drawing frame to LED strip hardware */
void WeUi_Ind_Adapter_DrawFrame(void);

#endif /* __WEUI_IND_ADAPTER_H */
