#include "weui_gfx_path.h"

/* Wang's formula for a cubic: n = sqrt(3 * 2 / 8 * |max second difference| / tolerance) */
#define WEUI_GFX_PATH_WANG_CUBIC	0.75f
/* Control arm of a quarter circle as one cubic: 4 / 3 * (sqrt(2) - 1) */
#define WEUI_GFX_PATH_KAPPA			0.5522847f
/* A quadratic's control raised to a cubic's two: 2 / 3 of the way from each end */
#define WEUI_GFX_PATH_QUAD_ELEV		(2.0f / 3.0f)
#define WEUI_GFX_PATH_SECOND_D		2.0f
/* The inner Bernstein weights of a cubic: 3 u^2 t and 3 u t^2 */
#define WEUI_GFX_PATH_BERNSTEIN_MID 3.0f

static RET_STATE_t WeUi_Gfx_Path_Fail(WeUi_Gfx_Path_t* pPath, RET_STATE_t rs) {
	pPath->Broken = true;
	return rs;
}

static WeUi_Gfx_PointF_t WeUi_Gfx_Path_Map(const WeUi_Gfx_Path_t* pcPath, float x, float y) {
	WeUi_Gfx_PointF_t pt = {x, y};
	return pcPath->pcXf ? WeUi_Gfx_Affine_Apply(pcPath->pcXf, pt) : pt;
}

static WeUi_Gfx_PathContour_t* WeUi_Gfx_Path_OpenContour(WeUi_Gfx_Path_t* pPath) {
	if (pPath->ContoursNum == 0)
		return NULL;

	WeUi_Gfx_PathContour_t* pContour = &pPath->Contours[pPath->ContoursNum - 1];
	return pContour->Closed ? NULL : pContour;
}

/* ceilf is a library call on some targets: the segment count is a small positive number */
static u32 WeUi_Gfx_Path_CeilPositive(float v) {
	u32 i = (u32)v;
	return i + ((float)i < v);
}

static void WeUi_Gfx_Path_Append(WeUi_Gfx_Path_t* pPath, WeUi_Gfx_PathContour_t* pContour,
								 WeUi_Gfx_PointF_t pt) {
	pPath->Pts[pPath->PtsNum++] = pt;
	pContour->Num++;
}

WeUi_Gfx_Shape_t WeUi_Gfx_Path_Shape(const WeUi_Gfx_Path_t* pcPath) {
	return (WeUi_Gfx_Shape_t){
		.pcPts		 = pcPath->Pts,
		.pcContours	 = pcPath->Contours,
		.ContoursNum = pcPath->ContoursNum,
	};
}

void WeUi_Gfx_Path_Reset(WeUi_Gfx_Path_t* pPath, const WeUi_Gfx_Affine_t* pcXf) {
	pPath->PtsNum	   = 0;
	pPath->ContoursNum = 0;
	pPath->Broken	   = false;
	pPath->pcXf		   = pcXf;
}

RET_STATE_t WeUi_Gfx_Path_MoveTo(WeUi_Gfx_Path_t* pPath, float x, float y) {
	if (pPath->ContoursNum >= WEUI_GFX_PATH_CONTOURS_MAX || pPath->PtsNum >= WEUI_GFX_PATH_PTS_MAX)
		return WeUi_Gfx_Path_Fail(pPath, RET_STATE_ERR_OVERFLOW);

	WeUi_Gfx_PathContour_t* pContour = &pPath->Contours[pPath->ContoursNum++];
	*pContour = (WeUi_Gfx_PathContour_t){.Start = pPath->PtsNum, .Num = 0, .Closed = false};
	WeUi_Gfx_Path_Append(pPath, pContour, WeUi_Gfx_Path_Map(pPath, x, y));
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Path_LineTo(WeUi_Gfx_Path_t* pPath, float x, float y) {
	WeUi_Gfx_PathContour_t* pContour = WeUi_Gfx_Path_OpenContour(pPath);
	if (pContour == NULL)
		return WeUi_Gfx_Path_Fail(pPath, RET_STATE_ERR_EMPTY);
	if (pPath->PtsNum >= WEUI_GFX_PATH_PTS_MAX)
		return WeUi_Gfx_Path_Fail(pPath, RET_STATE_ERR_OVERFLOW);

	WeUi_Gfx_Path_Append(pPath, pContour, WeUi_Gfx_Path_Map(pPath, x, y));
	return RET_STATE_SUCCESS;
}

/** @brief A cubic whose controls are on the panel already, flattened into the open contour */
static RET_STATE_t WeUi_Gfx_Path_CubicOnPanel(WeUi_Gfx_Path_t* pPath,
											  WeUi_Gfx_PathContour_t* pContour,
											  WeUi_Gfx_PointF_t p1, WeUi_Gfx_PointF_t p2,
											  WeUi_Gfx_PointF_t p3) {
	WeUi_Gfx_PointF_t p0 = pPath->Pts[pPath->PtsNum - 1];

	float ddx = fmaxf(fabsf(p0.X - WEUI_GFX_PATH_SECOND_D * p1.X + p2.X),
					  fabsf(p1.X - WEUI_GFX_PATH_SECOND_D * p2.X + p3.X));
	float ddy = fmaxf(fabsf(p0.Y - WEUI_GFX_PATH_SECOND_D * p1.Y + p2.Y),
					  fabsf(p1.Y - WEUI_GFX_PATH_SECOND_D * p2.Y + p3.Y));
	float dd  = sqrtf(ddx * ddx + ddy * ddy);
	u32 segs =
		WeUi_Gfx_Path_CeilPositive(sqrtf(dd * WEUI_GFX_PATH_WANG_CUBIC / WEUI_GFX_PATH_TOLERANCE));
	segs = CUT_MIN_MAX(segs, 1u, (u32)WEUI_GFX_PATH_CURVE_SEGS_MAX);

	if (pPath->PtsNum + segs > WEUI_GFX_PATH_PTS_MAX)
		return WeUi_Gfx_Path_Fail(pPath, RET_STATE_ERR_OVERFLOW);

	float step = 1.0f / (float)segs;
	for (u32 i = 1; i <= segs; i++) {
		float t	 = (float)i * step;
		float u	 = 1.0f - t;
		float b0 = u * u * u;
		float b1 = WEUI_GFX_PATH_BERNSTEIN_MID * u * u * t;
		float b2 = WEUI_GFX_PATH_BERNSTEIN_MID * u * t * t;
		float b3 = t * t * t;
		WeUi_Gfx_Path_Append(pPath, pContour,
							 (WeUi_Gfx_PointF_t){
								 .X = b0 * p0.X + b1 * p1.X + b2 * p2.X + b3 * p3.X,
								 .Y = b0 * p0.Y + b1 * p1.Y + b2 * p2.Y + b3 * p3.Y,
							 });
	}
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Path_CubicTo(WeUi_Gfx_Path_t* pPath, float x1, float y1, float x2, float y2,
								  float x, float y) {
	WeUi_Gfx_PathContour_t* pContour = WeUi_Gfx_Path_OpenContour(pPath);
	if (pContour == NULL)
		return WeUi_Gfx_Path_Fail(pPath, RET_STATE_ERR_EMPTY);

	return WeUi_Gfx_Path_CubicOnPanel(pPath, pContour, WeUi_Gfx_Path_Map(pPath, x1, y1),
									  WeUi_Gfx_Path_Map(pPath, x2, y2),
									  WeUi_Gfx_Path_Map(pPath, x, y));
}

RET_STATE_t WeUi_Gfx_Path_QuadTo(WeUi_Gfx_Path_t* pPath, float x1, float y1, float x, float y) {
	WeUi_Gfx_PathContour_t* pContour = WeUi_Gfx_Path_OpenContour(pPath);
	if (pContour == NULL)
		return WeUi_Gfx_Path_Fail(pPath, RET_STATE_ERR_EMPTY);

	WeUi_Gfx_PointF_t p0 = pPath->Pts[pPath->PtsNum - 1];
	WeUi_Gfx_PointF_t q	 = WeUi_Gfx_Path_Map(pPath, x1, y1);
	WeUi_Gfx_PointF_t p3 = WeUi_Gfx_Path_Map(pPath, x, y);
	WeUi_Gfx_PointF_t p1 = {p0.X + (q.X - p0.X) * WEUI_GFX_PATH_QUAD_ELEV,
							p0.Y + (q.Y - p0.Y) * WEUI_GFX_PATH_QUAD_ELEV};
	WeUi_Gfx_PointF_t p2 = {p3.X + (q.X - p3.X) * WEUI_GFX_PATH_QUAD_ELEV,
							p3.Y + (q.Y - p3.Y) * WEUI_GFX_PATH_QUAD_ELEV};
	return WeUi_Gfx_Path_CubicOnPanel(pPath, pContour, p1, p2, p3);
}

RET_STATE_t WeUi_Gfx_Path_Close(WeUi_Gfx_Path_t* pPath) {
	WeUi_Gfx_PathContour_t* pContour = WeUi_Gfx_Path_OpenContour(pPath);
	if (pContour == NULL)
		return WeUi_Gfx_Path_Fail(pPath, RET_STATE_ERR_EMPTY);

	pContour->Closed = true;
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Path_Ellipse(WeUi_Gfx_Path_t* pPath, float cx, float cy, float rx, float ry) {
	float kx = WEUI_GFX_PATH_KAPPA * rx;
	float ky = WEUI_GFX_PATH_KAPPA * ry;

	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_MoveTo(pPath, cx + rx, cy));
	RETURN_IF_UNSUCCESS(
		WeUi_Gfx_Path_CubicTo(pPath, cx + rx, cy + ky, cx + kx, cy + ry, cx, cy + ry));
	RETURN_IF_UNSUCCESS(
		WeUi_Gfx_Path_CubicTo(pPath, cx - kx, cy + ry, cx - rx, cy + ky, cx - rx, cy));
	RETURN_IF_UNSUCCESS(
		WeUi_Gfx_Path_CubicTo(pPath, cx - rx, cy - ky, cx - kx, cy - ry, cx, cy - ry));
	RETURN_IF_UNSUCCESS(
		WeUi_Gfx_Path_CubicTo(pPath, cx + kx, cy - ry, cx + rx, cy - ky, cx + rx, cy));
	return WeUi_Gfx_Path_Close(pPath);
}

RET_STATE_t WeUi_Gfx_Path_Rect(WeUi_Gfx_Path_t* pPath, float x0, float y0, float x1, float y1) {
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_MoveTo(pPath, x0, y0));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_LineTo(pPath, x1, y0));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_LineTo(pPath, x1, y1));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_LineTo(pPath, x0, y1));
	return WeUi_Gfx_Path_Close(pPath);
}
