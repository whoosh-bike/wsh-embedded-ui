#include "weui_gfx_raster.h"

/* Cells per chunk: a chunk no edge touched has one coverage across it */
#define WEUI_GFX_RASTER_CHUNK		8
/* Cells right of the region: a segment along its right edge spills into them */
#define WEUI_GFX_RASTER_PAD			2
#define WEUI_GFX_RASTER_STRIDE		(WEUI_GFX_RASTER_WIDTH_MAX + WEUI_GFX_RASTER_PAD)
/* Chunks over n cells, the last one partial */
#define WEUI_GFX_RASTER_CHUNKS(n)	(((n) + WEUI_GFX_RASTER_CHUNK - 1) / WEUI_GFX_RASTER_CHUNK)
#define WEUI_GFX_RASTER_CHUNKS_NUM	WEUI_GFX_RASTER_CHUNKS(WEUI_GFX_RASTER_STRIDE)
/* Full coverage in accumulator units: integer sums are much cheaper than float ones on an MCU */
#define WEUI_GFX_RASTER_ONE			65536.0f
/* From accumulator units down to 0..256, clamped to WEUI_GFX_RASTER_COVER_FULL */
#define WEUI_GFX_RASTER_COVER_SHIFT 8
#define WEUI_GFX_RASTER_HALF		0.5f
/* Past the cell a segment ends in, a slope inside one cell starts 1.5 cells from it */
#define WEUI_GFX_RASTER_NEXT_MID	1.5f
#define WEUI_GFX_RASTER_SPAN_CELLS	3 /* cells of a span with one full step in the middle */
#define WEUI_GFX_RASTER_RECT_PTS	4

/* Zero between shapes: the sweep clears every cell it reads. Cleared once before the first,
 * as WEUI_DATA_PLACEMENT may put them where the startup leaves noise */
static s32 WEUI_DATA_PLACEMENT Acc[WEUI_GFX_RASTER_BAND_ROWS][WEUI_GFX_RASTER_STRIDE];
/* Per row, the chunks an edge touched; elsewhere a row's coverage does not change */
static u8 WEUI_DATA_PLACEMENT Dirty[WEUI_GFX_RASTER_BAND_ROWS][WEUI_GFX_RASTER_CHUNKS_NUM];
static u8 WEUI_DATA_PLACEMENT RowCov[WEUI_GFX_RASTER_WIDTH_MAX];
static bool AccCleared;

/* floorf and ceilf are library calls on some targets */
static s32 WeUi_Gfx_Raster_Floor(float v) {
	s32 i = (s32)v;
	return i - (v < (float)i);
}

static s32 WeUi_Gfx_Raster_Ceil(float v) {
	s32 i = (s32)v;
	return i + (v > (float)i);
}

static float WeUi_Gfx_Raster_Clamp(float v, float lo, float hi) {
	return v < lo ? lo : (v > hi ? hi : v);
}

static void WeUi_Gfx_Raster_MarkDirty(u32 row, s32 xa, s32 xb) {
	for (s32 c = xa / WEUI_GFX_RASTER_CHUNK; c <= xb / WEUI_GFX_RASTER_CHUNK; c++)
		Dirty[row][c] = 1;
}

/**
 * @brief Spread the signed area of one segment over the cells of each row it crosses, in a
 *        region of w x h cells. What lies left of the region is moved onto its left edge, so
 *        it still counts in the running sum; what lies right of it lands in the padding.
 */
static void WeUi_Gfx_Raster_AccLine(s32 w, s32 h, float x0, float y0, float x1, float y1) {
	if (y0 == y1)
		return;

	float fw = (float)w;
	/* Split where the segment crosses a side, so each piece is clamped whole */
	if ((x0 < 0.0f && x1 > 0.0f) || (x0 > 0.0f && x1 < 0.0f)) {
		float ym = y0 + (0.0f - x0) * (y1 - y0) / (x1 - x0);
		WeUi_Gfx_Raster_AccLine(w, h, x0, y0, 0.0f, ym);
		WeUi_Gfx_Raster_AccLine(w, h, 0.0f, ym, x1, y1);
		return;
	}
	if ((x0 < fw && x1 > fw) || (x0 > fw && x1 < fw)) {
		float ym = y0 + (fw - x0) * (y1 - y0) / (x1 - x0);
		WeUi_Gfx_Raster_AccLine(w, h, x0, y0, fw, ym);
		WeUi_Gfx_Raster_AccLine(w, h, fw, ym, x1, y1);
		return;
	}

	x0		  = WeUi_Gfx_Raster_Clamp(x0, 0.0f, fw);
	x1		  = WeUi_Gfx_Raster_Clamp(x1, 0.0f, fw);
	float dir = 1.0f;
	if (y0 > y1) {
		float t = x0;
		x0		= x1;
		x1		= t;
		t		= y0;
		y0		= y1;
		y1		= t;
		dir		= -1.0f;
	}
	if (y1 <= 0.0f || y0 >= (float)h)
		return;

	/* The only divisions per segment: a float division is a library call on some targets */
	float dxdy = (x1 - x0) / (y1 - y0);
	float dydx = dxdy != 0.0f ? fabsf(1.0f / dxdy) : 0.0f;
	float x	   = x0;
	s32 y	   = (s32)y0;
	if (y0 < 0.0f) {
		x -= y0 * dxdy;
		y = 0;
	}
	s32 yEnd = GET_MIN(WeUi_Gfx_Raster_Ceil(y1), h);

	for (; y < yEnd; y++) {
		s32* pRow	= Acc[y];
		float dy	= fminf((float)(y + 1), y1) - fmaxf((float)y, y0);
		float xNext = WeUi_Gfx_Raster_Clamp(x + dxdy * dy, 0.0f, fw);
		float d		= dy * dir * WEUI_GFX_RASTER_ONE;
		float xa	= fminf(x, xNext);
		float xb	= fmaxf(x, xNext);
		s32 xai		= WeUi_Gfx_Raster_Floor(xa);
		s32 xbi		= WeUi_Gfx_Raster_Ceil(xb);
		float xaf	= (float)xai;

		WeUi_Gfx_Raster_MarkDirty((u32)y, xai, GET_MAX(xai + 1, xbi));

		if (xbi <= xai + 1) {
			/* Within one cell: its area splits between it and the next by where it runs */
			float xm = WEUI_GFX_RASTER_HALF * (x + xNext) - xaf;
			pRow[xai] += (s32)(d - d * xm);
			pRow[xai + 1] += (s32)(d * xm);
		} else {
			/* 1 / (xb - xa), where xb - xa = |dxdy| * dy */
			float sl  = dy >= 1.0f ? dydx : dydx / dy;
			float x0f = xa - xaf;
			float x1f = xb - (float)xbi + 1.0f;
			float a0  = WEUI_GFX_RASTER_HALF * sl * (1.0f - x0f) * (1.0f - x0f);
			float am  = WEUI_GFX_RASTER_HALF * sl * x1f * x1f;
			pRow[xai] += (s32)(d * a0);
			if (xbi == xai + 2) {
				pRow[xai + 1] += (s32)(d * (1.0f - a0 - am));
			} else {
				float a1 = sl * (WEUI_GFX_RASTER_NEXT_MID - x0f);
				pRow[xai + 1] += (s32)(d * (a1 - a0));
				s32 step = (s32)(d * sl);
				for (s32 xi = xai + 2; xi < xbi - 1; xi++)
					pRow[xi] += step;
				float full = (float)(xbi - xai - WEUI_GFX_RASTER_SPAN_CELLS) * sl;
				pRow[xbi - 1] += (s32)(d * (1.0f - (a1 + full) - am));
			}
			pRow[xbi] += (s32)(d * am);
		}
		x = xNext;
	}
}

static u8 WeUi_Gfx_Raster_Cover(s32 sum) {
	s32 v = (sum < 0 ? -sum : sum) >> WEUI_GFX_RASTER_COVER_SHIFT;
	return (u8)GET_MIN(v, WEUI_GFX_RASTER_COVER_FULL);
}

static void WeUi_Gfx_Raster_AccBand(const WeUi_Gfx_Shape_t* pcShape, s32 rx, s32 ry, s32 w, s32 h) {
	for (u32 c = 0; c < pcShape->ContoursNum; c++) {
		const WeUi_Gfx_PathContour_t* pcC = &pcShape->pcContours[c];
		const WeUi_Gfx_PointF_t* pcPts	  = &pcShape->pcPts[pcC->Start];

		for (u32 k = 0; k < pcC->Num; k++) {
			WeUi_Gfx_PointF_t a = pcPts[k];
			WeUi_Gfx_PointF_t b = pcPts[(k + 1) % pcC->Num];
			if (fmaxf(a.Y, b.Y) <= (float)ry || fminf(a.Y, b.Y) >= (float)(ry + h) ||
				fminf(a.X, b.X) >= (float)(rx + w))
				continue;

			WeUi_Gfx_Raster_AccLine(w, h, a.X - (float)rx, a.Y - (float)ry, b.X - (float)rx,
									b.Y - (float)ry);
		}
	}
}

/** @brief Sum one row into spans, clearing the cells it reads */
static void WeUi_Gfx_Raster_SweepRow(u32 r, s32 rx, s32 y, s32 w, WeUi_Gfx_Raster_SpanCb_t spanCb,
									 void* pCtx) {
	s32* pRow  = Acc[r];
	u8* pDirty = Dirty[r];
	s32 sum	   = 0;

	for (s32 x = 0; x < w;) {
		bool dirty = pDirty[x / WEUI_GFX_RASTER_CHUNK];
		s32 end	   = (x / WEUI_GFX_RASTER_CHUNK + 1) * WEUI_GFX_RASTER_CHUNK;
		while (end < w && (bool)pDirty[end / WEUI_GFX_RASTER_CHUNK] == dirty)
			end += WEUI_GFX_RASTER_CHUNK;
		end = GET_MIN(end, w);

		if (!dirty) {
			u8 cov = WeUi_Gfx_Raster_Cover(sum);
			if (cov > 0)
				spanCb(pCtx, (WeUi_Gfx_PtPos_t)y, (WeUi_Gfx_PtPos_t)(rx + x), (u16)(end - x), NULL,
					   cov);
			x = end;
			continue;
		}

		s32 start = x;
		for (; x < end; x++) {
			sum += pRow[x];
			pRow[x]	  = 0;
			RowCov[x] = WeUi_Gfx_Raster_Cover(sum);
		}
		spanCb(pCtx, (WeUi_Gfx_PtPos_t)y, (WeUi_Gfx_PtPos_t)(rx + start), (u16)(end - start),
			   &RowCov[start], 0);
	}

	pRow[w]		= 0;
	pRow[w + 1] = 0;
	memset(pDirty, 0, WEUI_GFX_RASTER_CHUNKS(w + WEUI_GFX_RASTER_PAD));
}

RET_STATE_t WeUi_Gfx_Raster_ShapeBounds(const WeUi_Gfx_Shape_t* pcShape,
										WeUi_Gfx_ClipRect_t* pBounds) {
	*pBounds   = (WeUi_Gfx_ClipRect_t){0};
	bool first = true;
	float minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f;

	for (u32 c = 0; c < pcShape->ContoursNum; c++) {
		const WeUi_Gfx_PathContour_t* pcC = &pcShape->pcContours[c];
		for (u32 k = 0; k < pcC->Num; k++) {
			WeUi_Gfx_PointF_t pt = pcShape->pcPts[pcC->Start + k];
			if (first) {
				minX = maxX = pt.X;
				minY = maxY = pt.Y;
				first		= false;
				continue;
			}
			minX = fminf(minX, pt.X);
			maxX = fmaxf(maxX, pt.X);
			minY = fminf(minY, pt.Y);
			maxY = fmaxf(maxY, pt.Y);
		}
	}
	if (first)
		return RET_STATE_SUCCESS;

	*pBounds = (WeUi_Gfx_ClipRect_t){
		.X0 = (WeUi_Gfx_PtPos_t)WeUi_Gfx_Raster_Floor(minX),
		.Y0 = (WeUi_Gfx_PtPos_t)WeUi_Gfx_Raster_Floor(minY),
		.X1 = (WeUi_Gfx_PtPos_t)WeUi_Gfx_Raster_Ceil(maxX),
		.Y1 = (WeUi_Gfx_PtPos_t)WeUi_Gfx_Raster_Ceil(maxY),
	};
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Raster_Bounds(const WeUi_Gfx_Path_t* pcPath, WeUi_Gfx_ClipRect_t* pBounds) {
	if (pcPath->Broken)
		return RET_STATE_ERR_PARAM;

	WeUi_Gfx_Shape_t shape = WeUi_Gfx_Path_Shape(pcPath);
	return WeUi_Gfx_Raster_ShapeBounds(&shape, pBounds);
}

/** @brief A shape of one rectangle whose sides lie on pixel edges: it covers its pixels whole */
static bool WeUi_Gfx_Raster_IsPixelRect(const WeUi_Gfx_Shape_t* pcShape) {
	if (pcShape->ContoursNum != 1 || pcShape->pcContours[0].Num != WEUI_GFX_RASTER_RECT_PTS)
		return false;

	const WeUi_Gfx_PointF_t* pcPts = &pcShape->pcPts[pcShape->pcContours[0].Start];
	/* Opposite corners apart both ways: four sides along the axes then make a rectangle */
	if (pcPts[0].X == pcPts[2].X || pcPts[0].Y == pcPts[2].Y || pcPts[1].X == pcPts[3].X ||
		pcPts[1].Y == pcPts[3].Y)
		return false;
	for (u32 k = 0; k < WEUI_GFX_RASTER_RECT_PTS; k++) {
		WeUi_Gfx_PointF_t a = pcPts[k];
		WeUi_Gfx_PointF_t b = pcPts[(k + 1) % WEUI_GFX_RASTER_RECT_PTS];
		if (a.X != (float)(s32)a.X || a.Y != (float)(s32)a.Y || (a.X == b.X) == (a.Y == b.Y))
			return false;
	}
	return true;
}

RET_STATE_t WeUi_Gfx_Raster_FillShape(const WeUi_Gfx_Shape_t* pcShape,
									  const WeUi_Gfx_ClipRect_t* pcClip,
									  WeUi_Gfx_Raster_SpanCb_t spanCb, void* pCtx) {
	if (pcClip->X1 - pcClip->X0 > WEUI_GFX_RASTER_WIDTH_MAX)
		return RET_STATE_ERR_PARAM;

	WeUi_Gfx_ClipRect_t shape;
	if (pcShape->pcBounds)
		shape = *pcShape->pcBounds;
	else
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Raster_ShapeBounds(pcShape, &shape));

	/* Left of the shape nothing adds up, so the region starts at it - or at the clip */
	s32 rx0 = GET_MAX(pcClip->X0, shape.X0);
	s32 ry0 = GET_MAX(pcClip->Y0, shape.Y0);
	s32 rx1 = GET_MIN(pcClip->X1, shape.X1);
	s32 ry1 = GET_MIN(pcClip->Y1, shape.Y1);
	if (rx0 >= rx1 || ry0 >= ry1)
		return RET_STATE_SUCCESS;

	/* No area to add up: a background, a bar */
	if (WeUi_Gfx_Raster_IsPixelRect(pcShape)) {
		for (s32 y = ry0; y < ry1; y++)
			spanCb(pCtx, (WeUi_Gfx_PtPos_t)y, (WeUi_Gfx_PtPos_t)rx0, (u16)(rx1 - rx0), NULL,
				   WEUI_GFX_RASTER_COVER_FULL);
		return RET_STATE_SUCCESS;
	}

	if (!AccCleared) {
		memset(Acc, 0, sizeof(Acc));
		memset(Dirty, 0, sizeof(Dirty));
		AccCleared = true;
	}

	s32 w = rx1 - rx0;
	for (s32 by = ry0; by < ry1; by += WEUI_GFX_RASTER_BAND_ROWS) {
		s32 bh = GET_MIN(WEUI_GFX_RASTER_BAND_ROWS, ry1 - by);
		WeUi_Gfx_Raster_AccBand(pcShape, rx0, by, w, bh);
		for (s32 r = 0; r < bh; r++)
			WeUi_Gfx_Raster_SweepRow((u32)r, rx0, by + r, w, spanCb, pCtx);
	}
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Raster_Fill(const WeUi_Gfx_Path_t* pcPath, const WeUi_Gfx_ClipRect_t* pcClip,
								 WeUi_Gfx_Raster_SpanCb_t spanCb, void* pCtx) {
	if (pcPath->Broken)
		return RET_STATE_ERR_PARAM;

	WeUi_Gfx_Shape_t shape = WeUi_Gfx_Path_Shape(pcPath);
	return WeUi_Gfx_Raster_FillShape(&shape, pcClip, spanCb, pCtx);
}
