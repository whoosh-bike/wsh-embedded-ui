#ifndef __WEUI_GFX_FRACTAL_TREE_H
#define __WEUI_GFX_FRACTAL_TREE_H

#include "weui_gfx_types.h"

/**
 * @brief One frame of a binary fractal tree. Every branch splits in two: one
 *        child turned counter-clockwise by SpreadCcw, the other clockwise by
 *        SpreadCw, both then turned by Sway. Equal spreads give a symmetric tree.
 *
 * Angles are in degrees, 0 pointing right and 90 straight up. Colour runs
 * linearly from Trunk at the first level to Tips at the last one.
 */
typedef struct {
	float RootX; /* where the trunk starts, panel coordinates */
	float RootY;
	float Length; /* trunk length, px */
	float Angle;  /* trunk direction */
	float SpreadCcw;
	float SpreadCw;
	float Sway;	  /* added to every branch, leans the crown */
	float Shrink; /* child length over parent length, (0; 1) */
	u32 Depth;	  /* levels drawn, trunk included */
	WeUi_Gfx_ColorSchemeHsb_t Trunk;
	WeUi_Gfx_ColorSchemeHsb_t Tips;
} WeUi_Gfx_FractalTreeCfg_t;

/** @brief Draw the tree into the drawing frame. Draws nothing for Depth 0. */
void WeUi_Gfx_FractalTree_Draw(const WeUi_Gfx_FractalTreeCfg_t* pcCfg);

#endif /* __WEUI_GFX_FRACTAL_TREE_H */
