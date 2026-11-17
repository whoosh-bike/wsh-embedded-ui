#ifndef __WEUI_GFX_EFFECT_STATIC_H
#define __WEUI_GFX_EFFECT_STATIC_H

#include "weui_gfx_effect_types.h"

/**
 * @brief One frame of TV static: every pixel a random full hue, or off. Covers
 *        every pixel, so a scene can call it straight without clearing first.
 */
void WeUi_Gfx_Effect_Static_Paint(void);

#endif /* __WEUI_GFX_EFFECT_STATIC_H */
