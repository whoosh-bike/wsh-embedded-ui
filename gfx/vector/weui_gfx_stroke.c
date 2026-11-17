#include "weui_gfx_stroke.h"

/* Points closer than this, |dx| + |dy| in pixels, are one: a zero-length edge has no direction */
#define WEUI_GFX_STROKE_SAME	   1e-3f
/* A turn gentler than this cosine (about 11 degrees) is a curve: one bisector point per side */
#define WEUI_GFX_STROKE_SMOOTH_COS 0.98f
#define WEUI_GFX_STROKE_CLOSED_MIN 3
#define WEUI_GFX_STROKE_HALF	   0.5f
#define WEUI_GFX_STROKE_FULL_TURN  (2.0f * M_PIf)
/* A miter's length over half the width is sqrt(2 / (1 + cos)) of the angle between normals */
#define WEUI_GFX_STROKE_MITER_NUM  2.0f
#define WEUI_GFX_STROKE_ARC_STEP   2.0f /* an arc step spans twice the angle of its sagitta */

typedef struct {
	WeUi_Gfx_Path_t* pOut;
	const WeUi_Gfx_StrokeStyle_t* pcStyle;
	float Half; /**< how far the side being built is offset */
	bool Started;
} WeUi_Gfx_StrokeCtx_t;

/* One contour at a time, its repeated points dropped */
static WeUi_Gfx_PointF_t WEUI_DATA_PLACEMENT Pts[WEUI_GFX_PATH_PTS_MAX];

static WeUi_Gfx_PointF_t WeUi_Gfx_Stroke_Add(WeUi_Gfx_PointF_t a, WeUi_Gfx_PointF_t b, float k) {
	return (WeUi_Gfx_PointF_t){a.X + b.X * k, a.Y + b.Y * k};
}

static float WeUi_Gfx_Stroke_Dot(WeUi_Gfx_PointF_t a, WeUi_Gfx_PointF_t b) {
	return a.X * b.X + a.Y * b.Y;
}

static float WeUi_Gfx_Stroke_Cross(WeUi_Gfx_PointF_t a, WeUi_Gfx_PointF_t b) {
	return a.X * b.Y - a.Y * b.X;
}

static float WeUi_Gfx_Stroke_Dist(WeUi_Gfx_PointF_t a, WeUi_Gfx_PointF_t b) {
	return sqrtf((b.X - a.X) * (b.X - a.X) + (b.Y - a.Y) * (b.Y - a.Y));
}

static WeUi_Gfx_PointF_t WeUi_Gfx_Stroke_Dir(WeUi_Gfx_PointF_t from, WeUi_Gfx_PointF_t to) {
	float len = WeUi_Gfx_Stroke_Dist(from, to);
	return (WeUi_Gfx_PointF_t){(to.X - from.X) / len, (to.Y - from.Y) / len};
}

/* The normal to the left of travel */
static WeUi_Gfx_PointF_t WeUi_Gfx_Stroke_Left(WeUi_Gfx_PointF_t dir) {
	return (WeUi_Gfx_PointF_t){-dir.Y, dir.X};
}

static bool WeUi_Gfx_Stroke_Same(WeUi_Gfx_PointF_t a, WeUi_Gfx_PointF_t b) {
	return fabsf(a.X - b.X) + fabsf(a.Y - b.Y) < WEUI_GFX_STROKE_SAME;
}

static RET_STATE_t WeUi_Gfx_Stroke_Put(WeUi_Gfx_StrokeCtx_t* pCtx, WeUi_Gfx_PointF_t pt) {
	if (pCtx->Started)
		return WeUi_Gfx_Path_LineTo(pCtx->pOut, pt.X, pt.Y);

	pCtx->Started = true;
	return WeUi_Gfx_Path_MoveTo(pCtx->pOut, pt.X, pt.Y);
}

/** @brief Points along an arc round c from the direction `from`, turning by `angle` */
static RET_STATE_t WeUi_Gfx_Stroke_Arc(WeUi_Gfx_StrokeCtx_t* pCtx, WeUi_Gfx_PointF_t c,
									   WeUi_Gfx_PointF_t from, float angle, float r) {
	/* Each step strays from the arc by at most the flattening tolerance */
	float step = r > WEUI_GFX_PATH_TOLERANCE
					 ? WEUI_GFX_STROKE_ARC_STEP * acosf(1.0f - WEUI_GFX_PATH_TOLERANCE / r)
					 : M_PIf;
	u32 steps  = (u32)(fabsf(angle) / step) + 1u;
	steps	   = GET_MIN(steps, (u32)WEUI_GFX_PATH_CURVE_SEGS_MAX);

	for (u32 k = 1; k <= steps; k++) {
		float a				  = angle * (float)k / (float)steps;
		float ca			  = cosf(a);
		float sa			  = sinf(a);
		WeUi_Gfx_PointF_t dir = {from.X * ca - from.Y * sa, from.X * sa + from.Y * ca};
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Add(c, dir, r)));
	}
	return RET_STATE_SUCCESS;
}

/** @brief The offset point where both edges' offsets meet: 1 / cos of half the turn away */
static WeUi_Gfx_PointF_t WeUi_Gfx_Stroke_Miter(WeUi_Gfx_PointF_t v, WeUi_Gfx_PointF_t n1,
											   WeUi_Gfx_PointF_t n2, float half) {
	WeUi_Gfx_PointF_t sum = {n1.X + n2.X, n1.Y + n2.Y};
	return WeUi_Gfx_Stroke_Add(v, sum, half / (1.0f + WeUi_Gfx_Stroke_Dot(n1, n2)));
}

/** @brief The side to the left of travel round the corner at v, coming from a, going to b */
static RET_STATE_t WeUi_Gfx_Stroke_Join(WeUi_Gfx_StrokeCtx_t* pCtx, WeUi_Gfx_PointF_t a,
										WeUi_Gfx_PointF_t v, WeUi_Gfx_PointF_t b) {
	WeUi_Gfx_PointF_t d1 = WeUi_Gfx_Stroke_Dir(a, v);
	WeUi_Gfx_PointF_t d2 = WeUi_Gfx_Stroke_Dir(v, b);
	WeUi_Gfx_PointF_t n1 = WeUi_Gfx_Stroke_Left(d1);
	WeUi_Gfx_PointF_t n2 = WeUi_Gfx_Stroke_Left(d2);
	float h				 = pCtx->Half;
	float cosTurn		 = WeUi_Gfx_Stroke_Dot(d1, d2);

	if (WeUi_Gfx_Stroke_Dot(d2, n1) >= 0.0f) {
		/* Turning towards this side: the offsets cross at the miter point, h tan(turn / 2)
		 * along each edge from the corner - when both edges are long enough for it */
		float reach = h * fabsf(WeUi_Gfx_Stroke_Cross(d1, d2)) / (1.0f + cosTurn);
		if (reach <= WEUI_GFX_STROKE_HALF * WeUi_Gfx_Stroke_Dist(a, v) &&
			reach <= WEUI_GFX_STROKE_HALF * WeUi_Gfx_Stroke_Dist(v, b))
			return WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Miter(v, n1, n2, h));

		/* Otherwise through the corner itself: a small loop, winding the same way */
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Add(v, n1, h)));
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Put(pCtx, v));
		return WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Add(v, n2, h));
	}

	if (cosTurn > WEUI_GFX_STROKE_SMOOTH_COS)
		return WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Miter(v, n1, n2, h));

	RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Add(v, n1, h)));

	const WeUi_Gfx_StrokeStyle_t* pcStyle = pCtx->pcStyle;
	if (pcStyle->Join == WEUI_GFX_JOIN_ROUND) {
		float turn = atan2f(WeUi_Gfx_Stroke_Cross(n1, n2), WeUi_Gfx_Stroke_Dot(n1, n2));
		return WeUi_Gfx_Stroke_Arc(pCtx, v, n1, turn, h);
	}
	if (pcStyle->Join == WEUI_GFX_JOIN_MITER) {
		float ratio = sqrtf(WEUI_GFX_STROKE_MITER_NUM / (1.0f + WeUi_Gfx_Stroke_Dot(n1, n2)));
		if (ratio <= pcStyle->MiterLimit)
			RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Miter(v, n1, n2, h)));
	}
	return WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Add(v, n2, h));
}

/** @brief From the left of travel at end e, round to its right, by the style's cap */
static RET_STATE_t WeUi_Gfx_Stroke_Cap(WeUi_Gfx_StrokeCtx_t* pCtx, WeUi_Gfx_PointF_t e,
									   WeUi_Gfx_PointF_t dir) {
	WeUi_Gfx_PointF_t n		 = WeUi_Gfx_Stroke_Left(dir);
	float h					 = pCtx->Half;
	WeUi_Gfx_PointF_t across = WeUi_Gfx_Stroke_Add(e, n, -h);

	if (pCtx->pcStyle->Cap == WEUI_GFX_CAP_ROUND) {
		float half = atan2f(WeUi_Gfx_Stroke_Cross(n, dir), WeUi_Gfx_Stroke_Dot(n, dir));
		return WeUi_Gfx_Stroke_Arc(pCtx, e, n, 2.0f * half, h);
	}
	if (pCtx->pcStyle->Cap == WEUI_GFX_CAP_SQUARE) {
		RETURN_IF_UNSUCCESS(
			WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Add(WeUi_Gfx_Stroke_Add(e, n, h), dir, h)));
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Add(across, dir, h)));
	}
	return WeUi_Gfx_Stroke_Put(pCtx, across);
}

static WeUi_Gfx_PointF_t WeUi_Gfx_Stroke_At(u32 i, u32 n, bool reversed) {
	return Pts[reversed ? n - 1u - i : i];
}

/** @brief The offset to the left of travel round a closed contour, as one closed polygon */
static RET_STATE_t WeUi_Gfx_Stroke_Loop(WeUi_Gfx_StrokeCtx_t* pCtx, u32 n, bool reversed,
										float half) {
	pCtx->Half	  = half;
	pCtx->Started = false;
	for (u32 i = 0; i < n; i++) {
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Join(
			pCtx, WeUi_Gfx_Stroke_At((i + n - 1u) % n, n, reversed),
			WeUi_Gfx_Stroke_At(i, n, reversed), WeUi_Gfx_Stroke_At((i + 1u) % n, n, reversed)));
	}
	return WeUi_Gfx_Path_Close(pCtx->pOut);
}

/** @brief Both sides of an open contour and its caps, as one closed polygon */
static RET_STATE_t WeUi_Gfx_Stroke_Open(WeUi_Gfx_StrokeCtx_t* pCtx, u32 n, float half) {
	WeUi_Gfx_PointF_t first = Pts[0];
	WeUi_Gfx_PointF_t last	= Pts[n - 1u];
	WeUi_Gfx_PointF_t dIn	= WeUi_Gfx_Stroke_Dir(first, Pts[1]);
	WeUi_Gfx_PointF_t dOut	= WeUi_Gfx_Stroke_Dir(Pts[n - 2u], last);
	pCtx->Half				= half;
	pCtx->Started			= false;

	RETURN_IF_UNSUCCESS(
		WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Add(first, WeUi_Gfx_Stroke_Left(dIn), half)));
	for (u32 i = 1; i + 1u < n; i++)
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Join(pCtx, Pts[i - 1u], Pts[i], Pts[i + 1u]));
	RETURN_IF_UNSUCCESS(
		WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Add(last, WeUi_Gfx_Stroke_Left(dOut), half)));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Cap(pCtx, last, dOut));

	/* Back along the other side: the left of travel the other way */
	for (u32 i = n - 2u; i >= 1u; i--)
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Join(pCtx, Pts[i + 1u], Pts[i], Pts[i - 1u]));
	WeUi_Gfx_PointF_t dBack = {-dIn.X, -dIn.Y};
	RETURN_IF_UNSUCCESS(
		WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Add(first, WeUi_Gfx_Stroke_Left(dBack), half)));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Cap(pCtx, first, dBack));
	return WeUi_Gfx_Path_Close(pCtx->pOut);
}

/** @brief A contour that is one point: a dot the size of the width, or nothing with butt caps */
static RET_STATE_t WeUi_Gfx_Stroke_Dot1(WeUi_Gfx_StrokeCtx_t* pCtx, float half) {
	static const WeUi_Gfx_PointF_t right = {1.0f, 0.0f};
	WeUi_Gfx_PointF_t c					 = Pts[0];
	pCtx->Half							 = half;
	pCtx->Started						 = false;

	if (pCtx->pcStyle->Cap == WEUI_GFX_CAP_BUTT)
		return RET_STATE_SUCCESS;

	if (pCtx->pcStyle->Cap == WEUI_GFX_CAP_ROUND) {
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Put(pCtx, WeUi_Gfx_Stroke_Add(c, right, half)));
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Arc(pCtx, c, right, WEUI_GFX_STROKE_FULL_TURN, half));
		return WeUi_Gfx_Path_Close(pCtx->pOut);
	}
	return WeUi_Gfx_Path_Rect(pCtx->pOut, c.X - half, c.Y - half, c.X + half, c.Y + half);
}

/** @brief Contour c into Pts, its repeated points dropped. @return how many are left */
static u32 WeUi_Gfx_Stroke_Copy(const WeUi_Gfx_Path_t* pcPath, u32 c, bool* pClosed) {
	const WeUi_Gfx_PathContour_t* pcC = &pcPath->Contours[c];
	u32 n							  = 0;

	for (u32 k = 0; k < pcC->Num; k++) {
		WeUi_Gfx_PointF_t pt = pcPath->Pts[pcC->Start + k];
		if (n > 0 && WeUi_Gfx_Stroke_Same(pt, Pts[n - 1u]))
			continue;
		Pts[n++] = pt;
	}
	*pClosed = pcC->Closed;
	if (*pClosed && n > 1u && WeUi_Gfx_Stroke_Same(Pts[n - 1u], Pts[0]))
		n--;
	if (n < WEUI_GFX_STROKE_CLOSED_MIN)
		*pClosed = false;
	return n;
}

/** @brief How much the style's coordinates scale a length, on the mean */
static float WeUi_Gfx_Stroke_Scale(const WeUi_Gfx_StrokeStyle_t* pcStyle) {
	const WeUi_Gfx_Affine_t* pcXf = pcStyle->pcXf;
	return pcXf ? sqrtf(fabsf(pcXf->A * pcXf->D - pcXf->B * pcXf->C)) : 1.0f;
}

WeUi_Gfx_StrokeStyle_t WeUi_Gfx_Stroke_Style(float width) {
	return (WeUi_Gfx_StrokeStyle_t){
		.Width		= width,
		.Join		= WEUI_GFX_JOIN_ROUND,
		.Cap		= WEUI_GFX_CAP_ROUND,
		.MiterLimit = WEUI_GFX_STROKE_MITER_LIMIT_DEF,
	};
}

RET_STATE_t WeUi_Gfx_Stroke_Build(const WeUi_Gfx_Path_t* pcPath,
								  const WeUi_Gfx_StrokeStyle_t* pcStyle, WeUi_Gfx_Path_t* pOut) {
	WeUi_Gfx_Path_Reset(pOut, NULL);
	float half = WEUI_GFX_STROKE_HALF * pcStyle->Width * WeUi_Gfx_Stroke_Scale(pcStyle);
	if (pcPath->Broken || !(half > 0.0f))
		return RET_STATE_ERR_PARAM;

	WeUi_Gfx_StrokeCtx_t ctx = {.pOut = pOut, .pcStyle = pcStyle};
	for (u32 c = 0; c < pcPath->ContoursNum; c++) {
		bool closed;
		u32 n = WeUi_Gfx_Stroke_Copy(pcPath, c, &closed);
		if (n == 0)
			continue;

		if (n == 1u) {
			RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Dot1(&ctx, half));
		} else if (closed) {
			/* A ring: the two sides wound opposite ways */
			RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Loop(&ctx, n, false, half));
			RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Loop(&ctx, n, true, half));
		} else {
			RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Open(&ctx, n, half));
		}
	}
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Stroke_Outline(const WeUi_Gfx_Path_t* pcPath,
									const WeUi_Gfx_StrokeStyle_t* pcStyle, WeUi_Gfx_Path_t* pOut) {
	WeUi_Gfx_Path_Reset(pOut, NULL);
	float width = pcStyle->Width * WeUi_Gfx_Stroke_Scale(pcStyle);
	if (pcPath->Broken || !(width > 0.0f))
		return RET_STATE_ERR_PARAM;

	WeUi_Gfx_StrokeCtx_t ctx = {.pOut = pOut, .pcStyle = pcStyle};
	for (u32 c = 0; c < pcPath->ContoursNum; c++) {
		bool closed;
		u32 n = WeUi_Gfx_Stroke_Copy(pcPath, c, &closed);
		if (!closed)
			continue;

		/* With y down, a positive area runs clockwise on the panel: its left is inside */
		float twice = 0.0f;
		for (u32 i = 0; i < n; i++)
			twice += WeUi_Gfx_Stroke_Cross(Pts[i], Pts[(i + 1u) % n]);
		bool leftIsIn = twice > 0.0f;

		/* Out by the width one way round, under the shape the other: a ring, not a disc */
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Loop(&ctx, n, leftIsIn, width));
		RETURN_IF_UNSUCCESS(
			WeUi_Gfx_Stroke_Loop(&ctx, n, !leftIsIn, WEUI_GFX_STROKE_OUTLINE_UNDER));
	}
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Stroke_OuterOutline(const WeUi_Gfx_Path_t* pcPath,
										 const WeUi_Gfx_StrokeStyle_t* pcStyle,
										 WeUi_Gfx_Path_t* pOut) {
	WeUi_Gfx_Path_Reset(pOut, NULL);
	float width = pcStyle->Width * WeUi_Gfx_Stroke_Scale(pcStyle);
	if (pcPath->Broken || !(width > 0.0f))
		return RET_STATE_ERR_PARAM;

	WeUi_Gfx_StrokeCtx_t ctx = {.pOut = pOut, .pcStyle = pcStyle};
	for (u32 c = 0; c < pcPath->ContoursNum; c++) {
		bool closed;
		u32 n = WeUi_Gfx_Stroke_Copy(pcPath, c, &closed);
		if (!closed)
			continue;

		/* With y down, a positive area runs clockwise on the panel: its left is inside */
		float twice = 0.0f;
		for (u32 i = 0; i < n; i++)
			twice += WeUi_Gfx_Stroke_Cross(Pts[i], Pts[(i + 1u) % n]);
		bool leftIsIn = twice > 0.0f;

		/* Out by the width one way round: the shape's fill hides what is under it */
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Loop(&ctx, n, leftIsIn, width));
	}
	return RET_STATE_SUCCESS;
}
