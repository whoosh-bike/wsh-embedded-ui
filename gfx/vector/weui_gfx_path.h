#ifndef __WEUI_GFX_PATH_H
#define __WEUI_GFX_PATH_H

#include "weui_gfx_affine.h"

/**
 * @file weui_gfx_path.h
 * @brief A shape's outline, built as in SVG (move, line, curves, close) and stored ready to
 *        rasterise: every point goes through the current transform as it is added, and
 *        curves are flattened right away into as few segments as keep them within
 *        WEUI_GFX_PATH_TOLERANCE of the true curve (Wang's formula). An affine map keeps a
 *        Bezier curve a Bezier curve, so curves are flattened on the panel, where the
 *        tolerance is in pixels whatever the scale.
 *
 * A path is built from scratch every frame and holds one shape: a few contours, closed
 * implicitly when filled. Building past its room fails the call and marks the path, so a
 * shape is never drawn with a part missing.
 */

#ifndef WEUI_GFX_PATH_PTS_MAX
#define WEUI_GFX_PATH_PTS_MAX 256
#endif /* WEUI_GFX_PATH_PTS_MAX */

#ifndef WEUI_GFX_PATH_CONTOURS_MAX
#define WEUI_GFX_PATH_CONTOURS_MAX 16
#endif /* WEUI_GFX_PATH_CONTOURS_MAX */

/** @brief Farthest a flattened curve strays from the true one, in panel pixels */
#ifndef WEUI_GFX_PATH_TOLERANCE
#define WEUI_GFX_PATH_TOLERANCE 0.2f
#endif /* WEUI_GFX_PATH_TOLERANCE */

/** @brief Segments one curve gets at most, however large it is drawn */
#ifndef WEUI_GFX_PATH_CURVE_SEGS_MAX
#define WEUI_GFX_PATH_CURVE_SEGS_MAX 48
#endif /* WEUI_GFX_PATH_CURVE_SEGS_MAX */

typedef struct {
	u16 Start; /**< index of its first point in Pts */
	u16 Num;
	bool Closed;
} WeUi_Gfx_PathContour_t;

/** @brief The pixels a shape may touch: X0, Y0 included, X1, Y1 not */
typedef struct {
	WeUi_Gfx_PtPos_t X0;
	WeUi_Gfx_PtPos_t Y0;
	WeUi_Gfx_PtPos_t X1;
	WeUi_Gfx_PtPos_t Y1;
} WeUi_Gfx_ClipRect_t;

/**
 * @brief Contours to fill, read where they lie - a path's, or a scene item's in the scene's
 *        pool - with no copy. Each contour's Start indexes pcPts.
 */
typedef struct {
	const WeUi_Gfx_PointF_t* pcPts;
	const WeUi_Gfx_PathContour_t* pcContours;
	u16 ContoursNum;
	/** WeUi_Gfx_Raster_ShapeBounds() of it worked out once - a scene's item is filled band by
	 *  band - or NULL: each fill goes over every point for them */
	const WeUi_Gfx_ClipRect_t* pcBounds;
} WeUi_Gfx_Shape_t;

typedef struct {
	WeUi_Gfx_PointF_t Pts[WEUI_GFX_PATH_PTS_MAX]; /**< on the panel, transformed */
	WeUi_Gfx_PathContour_t Contours[WEUI_GFX_PATH_CONTOURS_MAX];
	u16 PtsNum;
	u16 ContoursNum;
	bool Broken; /**< a call failed: the shape is incomplete */
	const WeUi_Gfx_Affine_t* pcXf;
} WeUi_Gfx_Path_t;

/**
 * @brief Empty the path for a new shape.
 * @param pcXf the transform every point added goes through, read on every call - pass a
 *        stack's &Top and the path follows it as parts are pushed and popped. NULL draws in
 *        panel coordinates.
 */
void WeUi_Gfx_Path_Reset(WeUi_Gfx_Path_t* pPath, const WeUi_Gfx_Affine_t* pcXf);

/** @brief The path's contours as a shape, read in place */
WeUi_Gfx_Shape_t WeUi_Gfx_Path_Shape(const WeUi_Gfx_Path_t* pcPath);

/**
 * The builders below return RET_STATE_ERR_OVERFLOW when the path has no room left
 * (WEUI_GFX_PATH_PTS_MAX, WEUI_GFX_PATH_CONTOURS_MAX) and RET_STATE_ERR_EMPTY for a line, a
 * curve or a close with no open contour - no MoveTo since the start or the last Close. Either
 * way nothing is added and the path is marked Broken.
 */

/** @brief Start a new contour at (x, y) */
RET_STATE_t WeUi_Gfx_Path_MoveTo(WeUi_Gfx_Path_t* pPath, float x, float y);
RET_STATE_t WeUi_Gfx_Path_LineTo(WeUi_Gfx_Path_t* pPath, float x, float y);

/** @brief Cubic Bezier from the last point through controls 1 and 2 to (x, y) */
RET_STATE_t WeUi_Gfx_Path_CubicTo(WeUi_Gfx_Path_t* pPath, float x1, float y1, float x2, float y2,
								  float x, float y);

/** @brief Quadratic Bezier from the last point through control 1 to (x, y) */
RET_STATE_t WeUi_Gfx_Path_QuadTo(WeUi_Gfx_Path_t* pPath, float x1, float y1, float x, float y);

/** @brief Join the contour's end back to its start: a stroke goes round, a fill is the same */
RET_STATE_t WeUi_Gfx_Path_Close(WeUi_Gfx_Path_t* pPath);

/** @brief A closed contour: four cubics, clockwise on the panel from (cx + rx, cy) */
RET_STATE_t WeUi_Gfx_Path_Ellipse(WeUi_Gfx_Path_t* pPath, float cx, float cy, float rx, float ry);

/** @brief A closed contour through the corners (x0, y0), (x1, y0), (x1, y1), (x0, y1) */
RET_STATE_t WeUi_Gfx_Path_Rect(WeUi_Gfx_Path_t* pPath, float x0, float y0, float x1, float y1);

#endif /* __WEUI_GFX_PATH_H */
