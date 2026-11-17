#ifndef __WEUI_GFX_EFFECT_CURTAIN_H
#define __WEUI_GFX_EFFECT_CURTAIN_H

#include "weui_gfx_effect_types.h"

/**
 * @brief Part the old picture down the middle like a curtain, both halves drawn
 *        off to the sides, the incoming scene standing behind it.
 *
 * @param axis  HORIZONTAL parts it left and right, VERTICAL up and down
 */
RET_STATE_t WeUi_Gfx_Effect_Curtain(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
									WEUI_GFX_EFFECT_AXIS_t axis);

#endif /* __WEUI_GFX_EFFECT_CURTAIN_H */
