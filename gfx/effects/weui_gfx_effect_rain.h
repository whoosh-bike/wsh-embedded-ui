#ifndef __WEUI_GFX_EFFECT_RAIN_H
#define __WEUI_GFX_EFFECT_RAIN_H

#include "weui_gfx_effect_types.h"

/**
 * @brief The incoming scene runs down column by column, each starting when it will.
 * @param columnPx  width of a column: 1 is a drizzle, wider falls in sheets
 */
RET_STATE_t WeUi_Gfx_Effect_Rain(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								 u8 columnPx);

#endif /* __WEUI_GFX_EFFECT_RAIN_H */
