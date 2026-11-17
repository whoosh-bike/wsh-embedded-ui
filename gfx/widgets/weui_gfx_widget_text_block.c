#include "weui_gfx_widget_text_block.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_text.h"

#define WEUI_GFX_WIDGET_TEXT_BLOCK_BORDER_PAD 1
#define WEUI_GFX_WIDGET_TEXT_BLOCK_LINE_GAP	  0

typedef struct {
	const WeUi_Gfx_Widget_TextBlock_Style_t* StylePtr;
	WeUi_Gfx_Text_t* TextObjPtr;
	WeUi_Gfx_PtPos_t RefX;
	WeUi_Gfx_PtPos_t RefY;
	WeUi_Gfx_PtPos_t X;
	WeUi_Gfx_ObjSize_t Width;
	u8 Ascent;
	u8 LineH;
	u16 LineCnt;
	bool IsDrawn;
} WeUi_Gfx_Widget_TextBlock_Ctx_t;

static void WeUi_Gfx_Widget_TextBlock_LineDraw(const WeUi_Gfx_Widget_TextBlock_Ctx_t* pcCtx,
											   const char* pcLine, u32 lineLen,
											   WeUi_Gfx_PtPos_t y) {
	if (!pcCtx->IsDrawn) {
		return;
	}

	WeUi_Gfx_Point_t Pt = {
		.X = pcCtx->RefX + pcCtx->X,
		.Y = pcCtx->RefY + y + pcCtx->Ascent,
	};
	WeUi_Gfx_Text_DrawAligned(&Pt, pcCtx->TextObjPtr, pcLine, lineLen, pcCtx->Width,
							  pcCtx->StylePtr->Align);
}

static void WeUi_Gfx_Widget_TextBlock_LineFeed(WeUi_Gfx_Widget_TextBlock_Ctx_t* pCtx,
											   const char* pcLine, u32 lineLen,
											   WeUi_Gfx_PtPos_t* pY) {
	if (pcLine) {
		WeUi_Gfx_Widget_TextBlock_LineDraw(pCtx, pcLine, lineLen, *pY);
	}
	*pY += pCtx->LineH + pCtx->StylePtr->LineGap;
	pCtx->LineCnt++;
}

static u32 WeUi_Gfx_Widget_TextBlock_CharLen(const char* pcStr, u32 len) {
	u32 charLen = WeUi_Gfx_Text_Utf8CharLen((u8)pcStr[0]);
	if (charLen > len) {
		charLen = len;
	}
	return charLen;
}

static void WeUi_Gfx_Widget_TextBlock_AddWord(WeUi_Gfx_Widget_TextBlock_Ctx_t* pCtx,
											  const char** ppcLine, u32* pLineLen,
											  WeUi_Gfx_PtPos_t* pY, const char* pcWord,
											  u32 wordLen) {
	if (*pLineLen > 0) {
		u32 tryLen = (u32)(pcWord + wordLen - *ppcLine);
		if (WeUi_Gfx_Text_GetPixLenN(pCtx->TextObjPtr, *ppcLine, tryLen) <= pCtx->Width) {
			*pLineLen = tryLen;
			return;
		}

		WeUi_Gfx_Widget_TextBlock_LineFeed(pCtx, *ppcLine, *pLineLen, pY);
		*ppcLine  = NULL;
		*pLineLen = 0;
	}

	u32 pos = 0;
	while (pos < wordLen) {
		u32 rem	   = wordLen - pos;
		u32 fitLen = WeUi_Gfx_Text_FitLen(pCtx->TextObjPtr, pcWord + pos, rem, pCtx->Width);
		if (!fitLen) {
			fitLen = WeUi_Gfx_Widget_TextBlock_CharLen(pcWord + pos, rem);
		}

		if (fitLen == rem) {
			*ppcLine  = pcWord + pos;
			*pLineLen = rem;
			return;
		}

		WeUi_Gfx_Widget_TextBlock_LineFeed(pCtx, pcWord + pos, fitLen, pY);
		pos += fitLen;
	}
}

static WeUi_Gfx_PtPos_t WeUi_Gfx_Widget_TextBlock_RunWrapped(WeUi_Gfx_Widget_TextBlock_Ctx_t* pCtx,
															 const char* pcStr,
															 WeUi_Gfx_PtPos_t yTop) {
	const char* pcLine = NULL;
	u32 lineLen		   = 0;
	WeUi_Gfx_PtPos_t y = yTop;

	const char* pcCur = pcStr;
	while (*pcCur) {
		while (*pcCur == ' ') {
			pcCur++;
		}
		if (!*pcCur) {
			break;
		}

		if (*pcCur == '\r' || *pcCur == '\n') {
			WeUi_Gfx_Widget_TextBlock_LineFeed(pCtx, pcLine, lineLen, &y);
			pcLine	= NULL;
			lineLen = 0;
			if (*pcCur == '\r' && pcCur[1] == '\n') {
				pcCur++;
			}
			pcCur++;
			continue;
		}

		const char* pcWord = pcCur;
		while (*pcCur && *pcCur != ' ' && *pcCur != '\r' && *pcCur != '\n') {
			pcCur++;
		}

		WeUi_Gfx_Widget_TextBlock_AddWord(pCtx, &pcLine, &lineLen, &y, pcWord,
										  (u32)(pcCur - pcWord));
	}

	if (lineLen > 0) {
		WeUi_Gfx_Widget_TextBlock_LineFeed(pCtx, pcLine, lineLen, &y);
	}

	return y;
}

static WeUi_Gfx_PtPos_t WeUi_Gfx_Widget_TextBlock_RunSingle(WeUi_Gfx_Widget_TextBlock_Ctx_t* pCtx,
															const char* pcStr,
															WeUi_Gfx_PtPos_t yTop) {
	WeUi_Gfx_PtPos_t y = yTop;
	u32 len			   = 0;

	while (pcStr[len] && pcStr[len] != '\n' && pcStr[len] != '\r') {
		len++;
	}

	u32 fitLen = WeUi_Gfx_Text_FitLen(pCtx->TextObjPtr, pcStr, len, pCtx->Width);
	if (!fitLen) {
		return y;
	}

	WeUi_Gfx_Widget_TextBlock_LineFeed(pCtx, pcStr, fitLen, &y);

	return y;
}

static WeUi_Gfx_PtPos_t WeUi_Gfx_Widget_TextBlock_Run(WeUi_Gfx_Widget_TextBlock_Ctx_t* pCtx,
													  const char* pcStr, WeUi_Gfx_PtPos_t yTop) {
	pCtx->LineCnt = 0;

	WeUi_Gfx_PtPos_t y = pCtx->StylePtr->IsWrapped
							 ? WeUi_Gfx_Widget_TextBlock_RunWrapped(pCtx, pcStr, yTop)
							 : WeUi_Gfx_Widget_TextBlock_RunSingle(pCtx, pcStr, yTop);

	if (pCtx->LineCnt == 0) {
		return yTop;
	}

	return y - pCtx->StylePtr->LineGap;
}

static void WeUi_Gfx_Widget_TextBlock_InitCtx(WeUi_Gfx_Widget_TextBlock_Ctx_t* pCtx,
											  WeUi_Gfx_Text_t* pTextObj,
											  const WeUi_Gfx_Widget_TextBlock_Style_t* pcStyle,
											  WeUi_Gfx_PtPos_t x, WeUi_Gfx_ObjSize_t w) {
	WeUi_Gfx_Text_InitStruct(pTextObj);
	pTextObj->Font		= pcStyle->FontPtr;
	pTextObj->TextColor = pcStyle->TextColor;
	pTextObj->BackColor = pcStyle->BgColor;

	WeUi_Gfx_ObjSize_t dispW = WeUi_Gfx_Adapter_GetWidth();
	if (!w) {
		w = (x < (WeUi_Gfx_PtPos_t)dispW) ? (WeUi_Gfx_ObjSize_t)(dispW - x) : 0;
	}

	pCtx->StylePtr	 = pcStyle;
	pCtx->TextObjPtr = pTextObj;
	pCtx->RefX		 = pcStyle->RefPtPtr ? pcStyle->RefPtPtr->X : 0;
	pCtx->RefY		 = pcStyle->RefPtPtr ? pcStyle->RefPtPtr->Y : 0;
	pCtx->X			 = x;
	pCtx->Width		 = w;
	pCtx->Ascent	 = WeUi_Gfx_Text_GetHeightAboveBaseline(pTextObj);
	pCtx->LineH		 = WeUi_Gfx_Text_GetHeight(pTextObj);
	pCtx->LineCnt	 = 0;
	pCtx->IsDrawn	 = false;
}

static WeUi_Gfx_ObjSize_t WeUi_Gfx_Widget_TextBlock_Measure(WeUi_Gfx_Widget_TextBlock_Ctx_t* pCtx,
															const char* pcStr) {
	bool isDrawn  = pCtx->IsDrawn;
	pCtx->IsDrawn = false;

	WeUi_Gfx_PtPos_t yEnd = WeUi_Gfx_Widget_TextBlock_Run(pCtx, pcStr, 0);

	pCtx->IsDrawn = isDrawn;

	return (yEnd > 0) ? (WeUi_Gfx_ObjSize_t)yEnd : 0;
}

static void WeUi_Gfx_Widget_TextBlock_PlateDraw(const WeUi_Gfx_Widget_TextBlock_Ctx_t* pcCtx,
												WeUi_Gfx_PtPos_t y, WeUi_Gfx_ObjSize_t h,
												WeUi_Gfx_Color_t color, bool isFilled) {
	WeUi_Gfx_Rect_t Plate;
	WeUi_Gfx_Rect_InitStruct(&Plate);
	Plate.Color	  = color;
	Plate.PtOne.X = pcCtx->RefX + pcCtx->X - WEUI_GFX_WIDGET_TEXT_BLOCK_BORDER_PAD;
	Plate.PtOne.Y = pcCtx->RefY + y - WEUI_GFX_WIDGET_TEXT_BLOCK_BORDER_PAD;
	Plate.Width	  = (WeUi_Gfx_ObjSize_t)(pcCtx->Width + 2 * WEUI_GFX_WIDGET_TEXT_BLOCK_BORDER_PAD);
	Plate.Height  = (WeUi_Gfx_ObjSize_t)(h + 2 * WEUI_GFX_WIDGET_TEXT_BLOCK_BORDER_PAD);

	if (isFilled) {
		WeUi_Gfx_RectEdgesFill_Draw(&Plate);
	} else {
		WeUi_Gfx_RectEdges_Draw(&Plate);
	}
}

void WeUi_Gfx_Widget_TextBlock_StyleInit(WeUi_Gfx_Widget_TextBlock_Style_t* pStyle,
										 const WeUi_Gfx_Point_t* pcRefPt, const u8* pcFont) {
	if (!pStyle) {
		return;
	}

	memset(pStyle, 0, sizeof(WeUi_Gfx_Widget_TextBlock_Style_t));
	memset(&pStyle->TextColor, WEUI_COLOR_CHANNEL_MAX, sizeof(pStyle->TextColor));
	memset(&pStyle->BgColor, WEUI_COLOR_CHANNEL_MIN, sizeof(pStyle->BgColor));

	pStyle->RefPtPtr  = pcRefPt;
	pStyle->FontPtr	  = pcFont;
	pStyle->LineGap	  = WEUI_GFX_WIDGET_TEXT_BLOCK_LINE_GAP;
	pStyle->Align	  = WEUI_TEXT_ALIGN_LEFT;
	pStyle->VAlign	  = WEUI_TEXT_VALIGN_TOP;
	pStyle->IsWrapped = true;
}

void WeUi_Gfx_Widget_TextBlock_StyleInvert(WeUi_Gfx_Widget_TextBlock_Style_t* pStyle) {
	if (!pStyle) {
		return;
	}

	WeUi_Gfx_Color_t Swap = pStyle->TextColor;
	pStyle->TextColor	  = pStyle->BgColor;
	pStyle->BgColor		  = Swap;
	pStyle->IsBgFilled	  = true;
}

WeUi_Gfx_ObjSize_t
WeUi_Gfx_Widget_TextBlock_GetHeight(const WeUi_Gfx_Widget_TextBlock_Style_t* pcStyle,
									const char* pcStr, WeUi_Gfx_ObjSize_t w) {
	if (!pcStyle || !pcStr || !pcStr[0]) {
		return 0;
	}

	WeUi_Gfx_Text_t TextObj;
	WeUi_Gfx_Widget_TextBlock_Ctx_t Ctx;
	WeUi_Gfx_Widget_TextBlock_InitCtx(&Ctx, &TextObj, pcStyle, 0, w);
	if (!Ctx.LineH) {
		return 0;
	}

	return WeUi_Gfx_Widget_TextBlock_Measure(&Ctx, pcStr);
}

WeUi_Gfx_PtPos_t WeUi_Gfx_Widget_TextBlock_Draw(const WeUi_Gfx_Widget_TextBlock_Style_t* pcStyle,
												const char* pcStr, WeUi_Gfx_PtPos_t x,
												WeUi_Gfx_PtPos_t y, WeUi_Gfx_ObjSize_t w,
												WeUi_Gfx_ObjSize_t h) {
	if (!pcStyle || !pcStr || !pcStr[0]) {
		return y;
	}

	WeUi_Gfx_Text_t TextObj;
	WeUi_Gfx_Widget_TextBlock_Ctx_t Ctx;
	WeUi_Gfx_Widget_TextBlock_InitCtx(&Ctx, &TextObj, pcStyle, x, w);
	if (!Ctx.LineH) {
		return y;
	}

	bool isPlated	= pcStyle->IsBgFilled || pcStyle->IsBordered;
	bool isVAligned = h && (pcStyle->VAlign != WEUI_TEXT_VALIGN_TOP);

	WeUi_Gfx_ObjSize_t blockH =
		(isPlated || isVAligned) ? WeUi_Gfx_Widget_TextBlock_Measure(&Ctx, pcStr) : 0;
	WeUi_Gfx_ObjSize_t boxH = h ? h : blockH;

	WeUi_Gfx_PtPos_t yTop = y;
	if (isVAligned && blockH < h) {
		WeUi_Gfx_ObjSize_t freeSpace = (WeUi_Gfx_ObjSize_t)(h - blockH);
		yTop += (pcStyle->VAlign == WEUI_TEXT_VALIGN_BOTTOM) ? (WeUi_Gfx_PtPos_t)freeSpace
															 : (WeUi_Gfx_PtPos_t)(freeSpace / 2);
	}

	if (pcStyle->IsBgFilled) {
		WeUi_Gfx_Widget_TextBlock_PlateDraw(&Ctx, y, boxH, pcStyle->BgColor, true);
	}

	Ctx.IsDrawn			  = true;
	WeUi_Gfx_PtPos_t yEnd = WeUi_Gfx_Widget_TextBlock_Run(&Ctx, pcStr, yTop);

	if (pcStyle->IsBordered) {
		WeUi_Gfx_Widget_TextBlock_PlateDraw(&Ctx, y, boxH, pcStyle->TextColor, false);
	}

	return yEnd;
}

void WeUi_Gfx_Widget_TextBlock_DrawScreenCentered(const WeUi_Gfx_Point_t* pcRefPt,
												  const char* pcStr, const u8* pcFont) {
	WeUi_Gfx_Widget_TextBlock_Style_t Style;
	WeUi_Gfx_Widget_TextBlock_StyleInit(&Style, pcRefPt, pcFont);
	Style.Align	 = WEUI_TEXT_ALIGN_CENTER;
	Style.VAlign = WEUI_TEXT_VALIGN_MIDDLE;

	WeUi_Gfx_Widget_TextBlock_Draw(&Style, pcStr, 0, 0, 0, WeUi_Gfx_Adapter_GetHeight());
}
