#ifndef __WEUI_GFX_EFFECT_RIPPLE_H
#define __WEUI_GFX_EFFECT_RIPPLE_H

#include "weui_gfx_effect_types.h"

/** @brief The ripple's outline */
typedef enum {
	WEUI_GFX_EFFECT_RIPPLE_CIRCLE = 0,
	WEUI_GFX_EFFECT_RIPPLE_DIAMOND, /* stretched to the panel, it reaches every corner at once */
} WEUI_GFX_EFFECT_RIPPLE_SHAPE_t;

/**
 * @brief Spread the incoming scene out of the centre like a ripple on water: a
 *        widening ring with a wavering, lit crest, the old picture outside it.
 */
RET_STATE_t WeUi_Gfx_Effect_Ripple(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								   WEUI_GFX_EFFECT_RIPPLE_SHAPE_t shape);

#endif /* __WEUI_GFX_EFFECT_RIPPLE_H */
