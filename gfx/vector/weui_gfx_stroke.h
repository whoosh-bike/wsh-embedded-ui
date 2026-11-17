#ifndef __WEUI_GFX_STROKE_H
#define __WEUI_GFX_STROKE_H

#include "weui_gfx_path.h"

/**
 * @file weui_gfx_stroke.h
 * @brief A path's stroke, or its outline, as another path - the area to fill, with any paint.
 *
 * Each contour becomes one polygon: its points offset to both sides, a join on the outer side
 * of every corner and the two sides of an open contour met by caps. On the inner side the
 * offsets meet at their miter point. Where an edge is too short for that - a wide stroke
 * turning sharply - they are joined through the corner itself instead: a small loop winding
 * the same way as the rest, which the nonzero fill covers with no hole, but whose partly
 * covered pixels come out a little heavy, as the rasteriser sums winding over a pixel. Where
 * a curve turns only a little (as flattened curves do) a point takes one offset point on the
 * outer side too, on the bisector, the way a smooth curve is offset.
 *
 * The stroke is built on the panel, from the path's points as they are stored; its width
 * comes from the style's own transform. Contour points are copied into a static buffer of
 * WEUI_GFX_PATH_PTS_MAX while they are stroked: one stroke at a time.
 */

/** @brief Corners sharper than this miter ratio (length over half the width) get a bevel */
#define WEUI_GFX_STROKE_MITER_LIMIT_DEF 4.0f

/** @brief How far an outline reaches under its shape, in panel pixels: the fill hides it */
#define WEUI_GFX_STROKE_OUTLINE_UNDER 1.5f

typedef enum {
	WEUI_GFX_JOIN_MITER = 0, /**< the sides meet in a point, up to the miter limit */
	WEUI_GFX_JOIN_ROUND,
	WEUI_GFX_JOIN_BEVEL, /**< the corner cut off */
} WEUI_GFX_JOIN_t;

typedef enum {
	WEUI_GFX_CAP_BUTT = 0, /**< square, at the end point */
	WEUI_GFX_CAP_ROUND,
	WEUI_GFX_CAP_SQUARE, /**< square, half the width past the end point */
} WEUI_GFX_CAP_t;

typedef struct {
	float Width;
	WEUI_GFX_JOIN_t Join;
	WEUI_GFX_CAP_t Cap;
	float MiterLimit; /**< below 1 every miter is a bevel */
	/**
	 * The coordinates Width is in: it is scaled by their mean scale, the square root of the
	 * determinant - a uniform scale exactly. NULL: panel pixels.
	 */
	const WeUi_Gfx_Affine_t* pcXf;
} WeUi_Gfx_StrokeStyle_t;

/** @brief Round joins and caps, the miter limit at its default, Width in panel pixels */
WeUi_Gfx_StrokeStyle_t WeUi_Gfx_Stroke_Style(float width);

/**
 * @brief Build the stroke of every contour of the path into pOut, which is reset first.
 *        A contour of one point is a dot with round or square caps and nothing with butt
 *        ones; a closed contour of fewer than three is stroked as an open one.
 * @return RET_STATE_ERR_PARAM for a Broken path or a width not above 0 (pOut is then left
 *         empty), and what building pOut returns when it runs out of room.
 */
RET_STATE_t WeUi_Gfx_Stroke_Build(const WeUi_Gfx_Path_t* pcPath,
								  const WeUi_Gfx_StrokeStyle_t* pcStyle, WeUi_Gfx_Path_t* pOut);

/**
 * @brief Build an outline of the path's closed contours into pOut: a ring Width wide outside
 *        each, reaching WEUI_GFX_STROKE_OUTLINE_UNDER under it, to be filled before the shape
 *        itself. Open contours have none. The style's Cap is not used.
 * @return as WeUi_Gfx_Stroke_Build().
 */
RET_STATE_t WeUi_Gfx_Stroke_Outline(const WeUi_Gfx_Path_t* pcPath,
									const WeUi_Gfx_StrokeStyle_t* pcStyle, WeUi_Gfx_Path_t* pOut);

#endif /* __WEUI_GFX_STROKE_H */
