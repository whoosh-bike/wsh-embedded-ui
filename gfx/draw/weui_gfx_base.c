#include "weui_gfx_base.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_transform.h"

void WeUi_Gfx_Point_InitStruct(WeUi_Gfx_Point_t* pPt) {
	memset((void*)pPt, 0, sizeof(WeUi_Gfx_Point_t));
}

void WeUi_Gfx_ColorPoint_InitStruct(WeUi_Gfx_ColorPoint_t* pCpt) {
	memset((void*)pCpt, 0, sizeof(WeUi_Gfx_ColorPoint_t));
}

void WeUi_Gfx_Base_InitStruct(WeUi_Gfx_Base_t* pBase) {
	memset((void*)pBase, 0, sizeof(WeUi_Gfx_Base_t));
}

void WeUi_Gfx_Line_InitStruct(WeUi_Gfx_Line_t* pLIne) {
	memset((void*)pLIne, 0, sizeof(WeUi_Gfx_Line_t));
	pLIne->Hatching = 1.0f;
}

void WeUi_Gfx_Circle_InitStruct(WeUi_Gfx_Circle_t* pCircle) {
	memset((void*)pCircle, 0, sizeof(WeUi_Gfx_Circle_t));
}

void WeUi_Gfx_Point_Fetch(WeUi_Gfx_ColorPoint_t* pCpt) {
	if (pCpt->Pt.X < WeUi_Gfx_Adapter_GetX_Min() || pCpt->Pt.X > WeUi_Gfx_Adapter_GetX_Max() ||
		pCpt->Pt.Y < WeUi_Gfx_Adapter_GetY_Min() || pCpt->Pt.Y > WeUi_Gfx_Adapter_GetY_Max()) {
		return;
	}

	pCpt->Color = WeUi_Gfx_Adapter_GetPixel(&(pCpt->Pt));
}

void WeUi_Gfx_Point_Draw(const WeUi_Gfx_ColorPoint_t* pcCpt) {
	if (pcCpt->Pt.X < WeUi_Gfx_Adapter_GetX_Min() || pcCpt->Pt.X > WeUi_Gfx_Adapter_GetX_Max() ||
		pcCpt->Pt.Y < WeUi_Gfx_Adapter_GetY_Min() || pcCpt->Pt.Y > WeUi_Gfx_Adapter_GetY_Max()) {
		return;
	}

	WeUi_Gfx_Adapter_SetPixel(pcCpt);
}

void WeUi_Gfx_LineHV_Draw(const WeUi_Gfx_ColorPoint_t* pcCpt, u16 len, WEUI_LINE_DIR_t dir) {
	WeUi_Gfx_ColorPoint_t pt = *pcCpt;

	for (u16 i = 0; i < len; i++) {
		if (dir == WEUI_LINE_DIR_HORIZONTAL) {
			pt.Pt.X = pcCpt->Pt.X + i;
		} else {
			pt.Pt.Y = pcCpt->Pt.Y + i;
		}

		WeUi_Gfx_Point_Draw(&pt);
	}
}

void WeUi_Gfx_Line_Draw(const WeUi_Gfx_Line_t* pcLine) {
	ASSERT_CHECK(pcLine->Hatching <= 1.0f);

	s16 dx	= ABS(pcLine->Base.PtTwo.X - pcLine->Base.PtOne.X);
	s16 dy	= ABS(pcLine->Base.PtTwo.Y - pcLine->Base.PtOne.Y);
	s16 sx	= (pcLine->Base.PtOne.X < pcLine->Base.PtTwo.X) ? 1 : -1;
	s16 sy	= (pcLine->Base.PtOne.Y < pcLine->Base.PtTwo.Y) ? 1 : -1;
	s16 err = ((dx > dy) ? dx : -dy) / 2;

	WeUi_Gfx_ColorPoint_t cpt = {
		.Pt.X  = pcLine->Base.PtOne.X,
		.Pt.Y  = pcLine->Base.PtOne.Y,
		.Color = pcLine->Base.Color,
	};

	u32 hatchThresh = (u32)(pcLine->Hatching * WEUI_HATCHING_PIXELS_SIZE_MAX);
	u32 drawPtCnt	= 0;

	while (true) {
		if ((drawPtCnt++ % WEUI_HATCHING_PIXELS_SIZE_MAX) < hatchThresh)
			WeUi_Gfx_Point_Draw(&cpt);

		if (cpt.Pt.X == pcLine->Base.PtTwo.X && cpt.Pt.Y == pcLine->Base.PtTwo.Y)
			break;

		s16 e2 = err;

		if (e2 > -dx) {
			err -= dy;
			cpt.Pt.X += sx;
		}

		if (e2 < dy) {
			err += dx;
			cpt.Pt.Y += sy;
		}
	}
}

void WeUi_Gfx_LineAngle_Draw(WeUi_Gfx_Line_t* pLine, u32 angle, u32 len) {
	float rad = D2R((float)angle);

	pLine->Base.PtTwo.X = pLine->Base.PtOne.X + (WeUi_Gfx_PtPos_t)roundf(cosf(rad) * (float)len);
	pLine->Base.PtTwo.Y = pLine->Base.PtOne.Y + (WeUi_Gfx_PtPos_t)roundf(sinf(rad) * (float)len);
	WeUi_Gfx_Line_Draw(pLine);
}

void WeUi_Gfx_RectVertices_Draw(const WeUi_Gfx_Rect_t* pcRect) {
	WeUi_Gfx_ColorPoint_t cpt = {.Color = pcRect->Color};
	WeUi_Gfx_PtPos_t x1		  = pcRect->PtOne.X;
	WeUi_Gfx_PtPos_t y1		  = pcRect->PtOne.Y;
	WeUi_Gfx_PtPos_t x2		  = pcRect->PtTwo.X;
	WeUi_Gfx_PtPos_t y2		  = pcRect->PtTwo.Y;

	u16 width  = ABS(x2 - x1) + 1;
	u16 height = ABS(y2 - y1) + 1;

	WeUi_Gfx_PtPos_t xMin = (x1 < x2) ? x1 : x2;
	WeUi_Gfx_PtPos_t yMin = (y1 < y2) ? y1 : y2;
	WeUi_Gfx_PtPos_t yMax = (y1 > y2) ? y1 : y2;

	// Top edge
	cpt.Pt.X = xMin;
	cpt.Pt.Y = yMin;
	WeUi_Gfx_LineHV_Draw(&cpt, width, WEUI_LINE_DIR_HORIZONTAL);

	// Bottom edge
	cpt.Pt.Y = yMax;
	WeUi_Gfx_LineHV_Draw(&cpt, width, WEUI_LINE_DIR_HORIZONTAL);

	// Left edge
	cpt.Pt.X = xMin;
	cpt.Pt.Y = yMin;
	WeUi_Gfx_LineHV_Draw(&cpt, height, WEUI_LINE_DIR_VERTICAL);

	// Right edge
	cpt.Pt.X = xMin + width - 1;
	WeUi_Gfx_LineHV_Draw(&cpt, height, WEUI_LINE_DIR_VERTICAL);
}

void WeUi_Gfx_RectEdges_Draw(const WeUi_Gfx_Rect_t* pcRect) {
	if (!pcRect->Width || !pcRect->Height)
		return;

	WeUi_Gfx_ColorPoint_t cpt = {.Color = pcRect->Color};

	// Top edge
	cpt.Pt.X = pcRect->PtOne.X;
	cpt.Pt.Y = pcRect->PtOne.Y;
	WeUi_Gfx_LineHV_Draw(&cpt, pcRect->Width, WEUI_LINE_DIR_HORIZONTAL);

	// Bottom edge
	cpt.Pt.Y = pcRect->PtOne.Y + pcRect->Height - 1;
	WeUi_Gfx_LineHV_Draw(&cpt, pcRect->Width, WEUI_LINE_DIR_HORIZONTAL);

	// Left edge
	cpt.Pt.X = pcRect->PtOne.X;
	cpt.Pt.Y = pcRect->PtOne.Y;
	WeUi_Gfx_LineHV_Draw(&cpt, pcRect->Height, WEUI_LINE_DIR_VERTICAL);

	// Right edge
	cpt.Pt.X = pcRect->PtOne.X + pcRect->Width - 1;
	WeUi_Gfx_LineHV_Draw(&cpt, pcRect->Height, WEUI_LINE_DIR_VERTICAL);
}

void WeUi_Gfx_RectVerticesFill_Draw(const WeUi_Gfx_Rect_t* pcRect) {
	WeUi_Gfx_ColorPoint_t cpt = {.Color = pcRect->Color};
	WeUi_Gfx_PtPos_t x1		  = pcRect->PtOne.X;
	WeUi_Gfx_PtPos_t y1		  = pcRect->PtOne.Y;
	WeUi_Gfx_PtPos_t x2		  = pcRect->PtTwo.X;
	WeUi_Gfx_PtPos_t y2		  = pcRect->PtTwo.Y;

	WeUi_Gfx_PtPos_t xMin = (x1 < x2) ? x1 : x2;
	WeUi_Gfx_PtPos_t yMin = (y1 < y2) ? y1 : y2;
	WeUi_Gfx_PtPos_t yMax = (y1 > y2) ? y1 : y2;
	u16 width			  = ABS(x2 - x1) + 1;

	for (cpt.Pt.Y = yMin; cpt.Pt.Y <= yMax; cpt.Pt.Y++) {
		cpt.Pt.X = xMin;
		WeUi_Gfx_LineHV_Draw(&cpt, width, WEUI_LINE_DIR_HORIZONTAL);
	}
}

void WeUi_Gfx_RectEdgesFill_Draw(const WeUi_Gfx_Rect_t* pcRect) {
	WeUi_Gfx_ColorPoint_t cpt = {
		.Color = pcRect->Color,
	};

	WeUi_Gfx_PtPos_t yEnd = pcRect->PtOne.Y + (WeUi_Gfx_PtPos_t)(pcRect->Height - 1);
	for (cpt.Pt.Y = pcRect->PtOne.Y; cpt.Pt.Y <= yEnd; cpt.Pt.Y++) {
		cpt.Pt.X = pcRect->PtOne.X;
		WeUi_Gfx_LineHV_Draw(&cpt, pcRect->Width, WEUI_LINE_DIR_HORIZONTAL);
	}
}

void WeUi_Gfx_Circle_Draw(const WeUi_Gfx_Circle_t* pcCircle, bool isFilled) {
	WeUi_Gfx_PtPos_t x = 0, y = pcCircle->Radius;
	s16 gap = 0, delta = (1 - 2 * pcCircle->Radius);

	while (y >= 0) {
		if (isFilled) {
			WeUi_Gfx_ColorPoint_t cpt = {.Color = pcCircle->Base.Color};

			cpt.Pt.X = pcCircle->Base.PtOne.X - x;
			cpt.Pt.Y = pcCircle->Base.PtOne.Y + y;
			WeUi_Gfx_LineHV_Draw(&cpt, 2 * x + 1, WEUI_LINE_DIR_HORIZONTAL);

			if (y != 0) {
				cpt.Pt.Y = pcCircle->Base.PtOne.Y - y;
				WeUi_Gfx_LineHV_Draw(&cpt, 2 * x + 1, WEUI_LINE_DIR_HORIZONTAL);
			}
		} else {
			WeUi_Gfx_ColorPoint_t cpt;

			cpt.Pt.X  = pcCircle->Base.PtOne.X + x;
			cpt.Pt.Y  = pcCircle->Base.PtOne.Y + y;
			cpt.Color = pcCircle->Base.Color;
			WeUi_Gfx_Point_Draw(&cpt);

			cpt.Pt.X  = pcCircle->Base.PtOne.X + x;
			cpt.Pt.Y  = pcCircle->Base.PtOne.Y - y;
			cpt.Color = pcCircle->Base.Color;
			WeUi_Gfx_Point_Draw(&cpt);

			cpt.Pt.X  = pcCircle->Base.PtOne.X - x;
			cpt.Pt.Y  = pcCircle->Base.PtOne.Y - y;
			cpt.Color = pcCircle->Base.Color;
			WeUi_Gfx_Point_Draw(&cpt);

			cpt.Pt.X  = pcCircle->Base.PtOne.X - x;
			cpt.Pt.Y  = pcCircle->Base.PtOne.Y + y;
			cpt.Color = pcCircle->Base.Color;
			WeUi_Gfx_Point_Draw(&cpt);
		}

		gap = 2 * (delta + y) - 1;

		if (delta < 0 && gap <= 0) {
			x++;
			delta += 2 * x + 1;
			continue;
		}

		if (delta > 0 && gap > 0) {
			y--;
			delta -= 2 * y + 1;
			continue;
		}

		x++;
		delta += 2 * (x - y);
		y--;
	}
}

void WeUi_Gfx_Frame_Fade(float keep) {
	ASSERT_CHECK(keep >= 0.0f && keep <= 1.0f);

	WeUi_Gfx_Point_t pt;
	for (pt.Y = WeUi_Gfx_Adapter_GetY_Min(); pt.Y <= WeUi_Gfx_Adapter_GetY_Max(); pt.Y++) {
		for (pt.X = WeUi_Gfx_Adapter_GetX_Min(); pt.X <= WeUi_Gfx_Adapter_GetX_Max(); pt.X++) {
			WeUi_Gfx_Color_t color =
				WeUi_Gfx_Transform_Bytes2Color(WeUi_Gfx_Adapter_GetPixelRaw(&pt));
			color.R = (WeUi_Gfx_ColorChannel_t)(color.R * keep);
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
			color.G = (WeUi_Gfx_ColorChannel_t)(color.G * keep);
			color.B = (WeUi_Gfx_ColorChannel_t)(color.B * keep);
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */
			WeUi_Gfx_Adapter_SetPixelRaw(&pt, WeUi_Gfx_Transform_Color2Bytes(color));
		}
	}
}
