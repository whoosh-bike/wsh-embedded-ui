#ifndef __WEUI_GFX_EFFECT_RELOAD_H
#define __WEUI_GFX_EFFECT_RELOAD_H

#include "weui_gfx_effect_types.h"

/** @brief The old picture is wiped off left to right, the incoming scene drawn in after it */
RET_STATE_t WeUi_Gfx_Effect_Reload(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames);

#endif /* __WEUI_GFX_EFFECT_RELOAD_H */
