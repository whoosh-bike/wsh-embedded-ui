#ifndef __WEUI_GFX_EFFECT_COVER_H
#define __WEUI_GFX_EFFECT_COVER_H

#include "weui_gfx_effect_types.h"

/** @brief Which picture of a cover moves - the other stands still */
typedef enum {
	WEUI_GFX_EFFECT_COVER_IN = 0, /* the incoming scene slides in over the old picture */
	WEUI_GFX_EFFECT_COVER_OFF,	  /* the old picture slides off the incoming scene */
} WEUI_GFX_EFFECT_COVER_t;

/** @brief One picture slides over the other, which stays where it is */
RET_STATE_t WeUi_Gfx_Effect_Cover(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								  WEUI_GFX_EFFECT_SLIDE_t slide, WEUI_GFX_EFFECT_COVER_t mode);

#endif /* __WEUI_GFX_EFFECT_COVER_H */
