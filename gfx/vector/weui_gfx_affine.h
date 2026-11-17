#ifndef __WEUI_GFX_AFFINE_H
#define __WEUI_GFX_AFFINE_H

#include "weui_gfx_types.h"

/**
 * @file weui_gfx_affine.h
 * @brief 2D affine transforms for the vector core: a point in float, a matrix, and a stack of
 *        them for nested parts (a pupil inside an eye inside a head). Coordinates are the
 *        panel's: x to the right, y down, so a positive angle turns clockwise on the panel.
 *        3D points (the cube demo) go through WeUi_Gfx_Transform_Rotate/Offset/Scale instead.
 */

#ifndef WEUI_GFX_AFFINE_STACK_DEPTH
#define WEUI_GFX_AFFINE_STACK_DEPTH 8
#endif /* WEUI_GFX_AFFINE_STACK_DEPTH */

/** @brief A point between pixels. WeUi_Gfx_Point_t stays the integer pixel. */
typedef struct {
	float X;
	float Y;
} WeUi_Gfx_PointF_t;

/** @brief x' = A x + C y + E, y' = B x + D y + F - the layout of SVG's matrix(a b c d e f) */
typedef struct {
	float A;
	float B;
	float C;
	float D;
	float E;
	float F;
} WeUi_Gfx_Affine_t;

/**
 * @brief The current transform and the ones saved under it. Owned by the caller - a scene
 *        keeps one - so the library holds no transform state of its own.
 */
typedef struct {
	WeUi_Gfx_Affine_t Top;
	WeUi_Gfx_Affine_t Saved[WEUI_GFX_AFFINE_STACK_DEPTH];
	u8 Depth;
} WeUi_Gfx_AffineStack_t;

WeUi_Gfx_Affine_t WeUi_Gfx_Affine_Identity(void);
WeUi_Gfx_Affine_t WeUi_Gfx_Affine_Translate(float x, float y);
WeUi_Gfx_Affine_t WeUi_Gfx_Affine_Scale(float x, float y);
WeUi_Gfx_Affine_t WeUi_Gfx_Affine_Rotate(float rad);

/** @brief m * n: the transform that applies n first, then m */
WeUi_Gfx_Affine_t WeUi_Gfx_Affine_Mul(const WeUi_Gfx_Affine_t* pcM, const WeUi_Gfx_Affine_t* pcN);

WeUi_Gfx_PointF_t WeUi_Gfx_Affine_Apply(const WeUi_Gfx_Affine_t* pcM, WeUi_Gfx_PointF_t pt);

/**
 * @brief The transform that undoes m, for taking a panel point back into a shape's own
 *        coordinates (a touch on a part, a gradient across it).
 * @return RET_STATE_ERR_PARAM when m flattens the plane (a zero scale) and has no inverse;
 *         pInv is left as it was then.
 */
RET_STATE_t WeUi_Gfx_Affine_Invert(const WeUi_Gfx_Affine_t* pcM, WeUi_Gfx_Affine_t* pInv);

/** @brief Identity on top, nothing saved */
void WeUi_Gfx_AffineStack_Reset(WeUi_Gfx_AffineStack_t* pStack);

/** @return RET_STATE_ERR_OVERFLOW when WEUI_GFX_AFFINE_STACK_DEPTH are saved already */
RET_STATE_t WeUi_Gfx_AffineStack_Push(WeUi_Gfx_AffineStack_t* pStack);

/** @return RET_STATE_ERR_EMPTY when nothing is saved; the top is kept then */
RET_STATE_t WeUi_Gfx_AffineStack_Pop(WeUi_Gfx_AffineStack_t* pStack);

/**
 * @brief Each of these puts its transform under the top one: it acts on what is drawn next in
 *        the coordinates set up so far, as in SVG and canvas.
 */
void WeUi_Gfx_AffineStack_Concat(WeUi_Gfx_AffineStack_t* pStack, const WeUi_Gfx_Affine_t* pcM);
void WeUi_Gfx_AffineStack_Translate(WeUi_Gfx_AffineStack_t* pStack, float x, float y);
void WeUi_Gfx_AffineStack_Scale(WeUi_Gfx_AffineStack_t* pStack, float x, float y);
void WeUi_Gfx_AffineStack_Rotate(WeUi_Gfx_AffineStack_t* pStack, float rad);

#endif /* __WEUI_GFX_AFFINE_H */
