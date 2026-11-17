#ifndef __WEUI_GFX_EFFECT_INTERLACE_H
#define __WEUI_GFX_EFFECT_INTERLACE_H

#include "weui_gfx_effect_types.h"

/** @brief The incoming scene slides in row by row, alternate rows from either side */
RET_STATE_t WeUi_Gfx_Effect_Interlace(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames);

#endif /* __WEUI_GFX_EFFECT_INTERLACE_H */
