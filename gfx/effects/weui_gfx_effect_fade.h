#ifndef __WEUI_GFX_EFFECT_FADE_H
#define __WEUI_GFX_EFFECT_FADE_H

#include "weui_gfx_effect_types.h"

/** @brief Cross-fade: the old picture melts into the incoming scene, mixed pixel by pixel */
RET_STATE_t WeUi_Gfx_Effect_Fade(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames);

#endif /* __WEUI_GFX_EFFECT_FADE_H */
