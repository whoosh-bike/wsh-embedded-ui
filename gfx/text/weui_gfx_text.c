#include "weui_gfx_text.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"

// Return codes of u8x8_utf8_next(): the string ended, or the sequence needs more bytes
#define WEUI_GFX_TEXT_UTF8_END	0x0FFFF
#define WEUI_GFX_TEXT_UTF8_MORE 0x0FFFE

// Highest code point, and the range reserved for UTF-16 surrogates, which UTF-8 cannot carry
#define WEUI_GFX_TEXT_UTF8_MAX_CP	  0x10FFFF
#define WEUI_GFX_TEXT_UTF8_SURR_FIRST 0xD800
#define WEUI_GFX_TEXT_UTF8_SURR_LAST  0xDFFF

// Continuation bytes carry 6 bits under the 10xxxxxx marker
#define WEUI_GFX_TEXT_UTF8_CONT_BITS 6
#define WEUI_GFX_TEXT_UTF8_CONT_MASK 0x3F
#define WEUI_GFX_TEXT_UTF8_CONT_MARK 0x80

// First code point that no longer fits in the shorter sequence
#define WEUI_GFX_TEXT_UTF8_2BYTE_FIRST 0x80
#define WEUI_GFX_TEXT_UTF8_3BYTE_FIRST 0x800
#define WEUI_GFX_TEXT_UTF8_4BYTE_FIRST 0x10000

// Lead byte marker: 0xxxxxxx, 110xxxxx, 1110xxxx, 11110xxx, so the ones before the
// first zero tell the decoder how many bytes the sequence takes
#define WEUI_GFX_TEXT_UTF8_LEAD_1BYTE 0x00
#define WEUI_GFX_TEXT_UTF8_LEAD_2BYTE 0xC0
#define WEUI_GFX_TEXT_UTF8_LEAD_3BYTE 0xE0
#define WEUI_GFX_TEXT_UTF8_LEAD_4BYTE 0xF0

static u8g2_t Font;
static WeUi_Gfx_Color_t ActiveTextColor;
static WeUi_Gfx_Color_t ActiveBackColor;
static u8x8_display_info_t DisplayInfo;

// u8g2 callback. DO NOT CHANGE the function signature!
// dir: 0 = horizontal (left to right), 1 = vertical (top to bottom)
static void WeUi_Gfx_Text_DrawHVLine(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len,
									 uint8_t dir) {
	WeUi_Gfx_ColorPoint_t cpt = {
		.Pt.X  = x,
		.Pt.Y  = y,
		.Color = u8g2->draw_color ? ActiveTextColor : ActiveBackColor,
	};

	WeUi_Gfx_LineHV_Draw(&cpt, len, dir);
}

void WeUi_Gfx_Text_Init(void) {
	memset(&ActiveTextColor, WEUI_COLOR_CHANNEL_MAX, sizeof(ActiveTextColor));
	memset(&ActiveBackColor, WEUI_COLOR_CHANNEL_MIN, sizeof(ActiveBackColor));
	memset(&Font, 0x00, sizeof(Font));

	u16 dispHeight	  = WeUi_Gfx_Adapter_GetHeight();
	u8 tileBuffHeight = (dispHeight + 7) / 8;

	// tile_width is not used for actual X clipping (that relies on pixel_width),
	// but u8g2 requires it to compute pixel_buf_width internally. 16 tiles = 128px.
	DisplayInfo.tile_width	 = 16;
	DisplayInfo.tile_height	 = tileBuffHeight;
	DisplayInfo.pixel_width	 = WeUi_Gfx_Adapter_GetWidth();
	DisplayInfo.pixel_height = dispHeight;
	Font.u8x8.display_info	 = &DisplayInfo;

	// tile_buf_ptr is not used in this simplified port, but tile_buf_height
	// controls the Y clipping window: buf_y1 = tile_buf_height * 8
	u8g2_SetupBuffer(&Font, NULL, tileBuffHeight, WeUi_Gfx_Text_DrawHVLine, &u8g2_cb_r0);
	u8g2_SetDrawColor(&Font, 1);
}

void WeUi_Gfx_Text_InitStruct(WeUi_Gfx_Text_t* pTextObj) {
	if (!pTextObj)
		return;

	pTextObj->Str	   = NULL;
	pTextObj->Font	   = NULL;
	pTextObj->BgMode   = WEUI_TEXT_BG_MODE_TRANSPARENT;
	pTextObj->Rotation = WEUI_TEXT_ROTATION_NONE;
	memset(&pTextObj->TextColor, WEUI_COLOR_CHANNEL_MAX, sizeof(WeUi_Gfx_Color_t));
	memset(&pTextObj->BackColor, WEUI_COLOR_CHANNEL_MIN, sizeof(WeUi_Gfx_Color_t));
}

/**
 * @return false while no font is set, by this object or an earlier one: u8g2 would
 *         read its glyph tables from address 0
 */
static bool WeUi_Gfx_Text_ApplySettings(WeUi_Gfx_Text_t* pTextObj) {
	if (pTextObj->Font)
		u8g2_SetFont(&Font, pTextObj->Font);

	u8g2_SetFontMode(&Font, pTextObj->BgMode);
	u8g2_SetFontDirection(&Font, pTextObj->Rotation);

	return Font.font != NULL;
}

/**
 * @brief Decode the next glyph of pcStr[*pPos; len) and step *pPos past its bytes.
 *        Call u8x8_utf8_init() before the first one
 * @return false once the run is over, leaving *pEnc untouched
 */
static bool WeUi_Gfx_Text_GlyphNext(const char* pcStr, u32 len, u32* pPos, u16* pEnc) {
	while (*pPos < len) {
		u16 enc = u8x8_utf8_next(u8g2_GetU8x8(&Font), (u8)pcStr[*pPos]);
		(*pPos)++;

		if (enc == WEUI_GFX_TEXT_UTF8_END) {
			break;
		}
		if (enc == WEUI_GFX_TEXT_UTF8_MORE) {
			continue;
		}

		*pEnc = enc;
		return true;
	}

	return false;
}

// Mirrors u8g2_string_width(), which stops at the terminator instead of a length:
// the trailing glyph contributes its ink width rather than its advance width, and
// the leading one its x offset
static u8g2_uint_t WeUi_Gfx_Text_WidthN(const char* pcStr, u32 len) {
	u8x8_utf8_init(u8g2_GetU8x8(&Font));
	Font.font_decode.glyph_width = 0;

	u8g2_uint_t w	= 0;
	u8g2_uint_t dx	= 0;
	s8 firstXOffset = 0;
	bool isFirst	= true;
	u32 pos			= 0;
	u16 enc			= 0;

	while (WeUi_Gfx_Text_GlyphNext(pcStr, len, &pos, &enc)) {
		dx = (u8g2_uint_t)(u8)u8g2_GetGlyphWidth(&Font, enc);
		if (isFirst) {
			firstXOffset = Font.glyph_x_offset;
			isFirst		 = false;
		}
		w += dx;
	}

	if (Font.font_decode.glyph_width) {
		w -= dx;
		w += Font.font_decode.glyph_width;
		w += Font.glyph_x_offset;
		if (firstXOffset > 0) {
			w += firstXOffset;
		}
	}

	return w;
}

// Mirrors u8g2_draw_string(), bounded by a length instead of the terminator
static void WeUi_Gfx_Text_DrawN(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y, const char* pcStr,
								u32 len) {
	u8x8_utf8_init(u8g2_GetU8x8(&Font));

	u32 pos = 0;
	u16 enc = 0;

	while (WeUi_Gfx_Text_GlyphNext(pcStr, len, &pos, &enc)) {
		u8g2_uint_t delta = u8g2_DrawGlyph(&Font, x, y, enc);

		switch ((WEUI_TEXT_ROTATION_MODE_t)Font.font_decode.dir) {
			case WEUI_TEXT_ROTATION_NONE:
				x += delta;
				break;
			case WEUI_TEXT_ROTATION_90:
				y += delta;
				break;
			case WEUI_TEXT_ROTATION_180:
				x -= delta;
				break;
			case WEUI_TEXT_ROTATION_270:
				y -= delta;
				break;
			default:
				break;
		}
	}
}

RET_STATE_t WeUi_Gfx_Text_Draw(WeUi_Gfx_Point_t* pRefPt, WeUi_Gfx_Text_t* pTextObj) {
	if (!pRefPt || !pTextObj || !pTextObj->Str || !WeUi_Gfx_Text_ApplySettings(pTextObj)) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	ActiveTextColor = pTextObj->TextColor;
	ActiveBackColor = pTextObj->BackColor;
	u8g2_DrawUTF8(&Font, pRefPt->X, pRefPt->Y, pTextObj->Str);

	return RET_STATE_SUCCESS;
}

u32 WeUi_Gfx_Text_GetPixLen(WeUi_Gfx_Text_t* pTextObj) {
	if (!pTextObj || !pTextObj->Str || !WeUi_Gfx_Text_ApplySettings(pTextObj))
		return 0;

	return u8g2_GetUTF8Width(&Font, pTextObj->Str);
}

u32 WeUi_Gfx_Text_GetPixLenN(WeUi_Gfx_Text_t* pTextObj, const char* pcStr, u32 len) {
	if (!pTextObj || !pcStr || !len || !WeUi_Gfx_Text_ApplySettings(pTextObj)) {
		return 0;
	}

	return WeUi_Gfx_Text_WidthN(pcStr, len);
}

u32 WeUi_Gfx_Text_FitLen(WeUi_Gfx_Text_t* pTextObj, const char* pcStr, u32 len,
						 WeUi_Gfx_ObjSize_t maxWidth) {
	if (!pTextObj || !pcStr || !len || !WeUi_Gfx_Text_ApplySettings(pTextObj)) {
		return 0;
	}

	u8x8_utf8_init(u8g2_GetU8x8(&Font));
	Font.font_decode.glyph_width = 0;

	u8g2_uint_t sumAdv = 0;
	u32 fitLen		   = 0;
	s8 firstXOffset	   = 0;
	bool isFirst	   = true;
	u32 pos			   = 0;
	u16 enc			   = 0;

	while (WeUi_Gfx_Text_GlyphNext(pcStr, len, &pos, &enc)) {
		u8g2_uint_t dx = (u8g2_uint_t)(u8)u8g2_GetGlyphWidth(&Font, enc);
		if (isFirst) {
			firstXOffset = Font.glyph_x_offset;
			isFirst		 = false;
		}

		u8g2_uint_t w = sumAdv + dx;
		if (Font.font_decode.glyph_width) {
			w -= dx;
			w += Font.font_decode.glyph_width;
			w += Font.glyph_x_offset;
			if (firstXOffset > 0) {
				w += firstXOffset;
			}
		}

		if (w > maxWidth) {
			break;
		}

		sumAdv += dx;
		fitLen = pos;
	}

	return fitLen;
}

RET_STATE_t WeUi_Gfx_Text_DrawAligned(WeUi_Gfx_Point_t* pRefPt, WeUi_Gfx_Text_t* pTextObj,
									  const char* pcStr, u32 len, WeUi_Gfx_ObjSize_t width,
									  WEUI_TEXT_ALIGN_t align) {
	if (!pRefPt || !pTextObj || !pcStr || !WeUi_Gfx_Text_ApplySettings(pTextObj)) {
		PANIC();
		return RET_STATE_ERR_PARAM;
	}

	if (!len) {
		return RET_STATE_SUCCESS;
	}

	ActiveTextColor = pTextObj->TextColor;
	ActiveBackColor = pTextObj->BackColor;

	WeUi_Gfx_PtPos_t x = pRefPt->X;
	if (align != WEUI_TEXT_ALIGN_LEFT) {
		u8g2_uint_t lineW = WeUi_Gfx_Text_WidthN(pcStr, len);
		if (lineW < width) {
			WeUi_Gfx_ObjSize_t freeSpace = (WeUi_Gfx_ObjSize_t)(width - lineW);
			x += (align == WEUI_TEXT_ALIGN_RIGHT) ? (WeUi_Gfx_PtPos_t)freeSpace
												  : (WeUi_Gfx_PtPos_t)(freeSpace / 2);
		}
	}

	WeUi_Gfx_Text_DrawN(x, pRefPt->Y, pcStr, len);

	return RET_STATE_SUCCESS;
}

u8 WeUi_Gfx_Text_GetHeight(WeUi_Gfx_Text_t* pTextObj) {
	if (!pTextObj || !WeUi_Gfx_Text_ApplySettings(pTextObj))
		return 0;

	return Font.font_info.max_char_height;
}

u8 WeUi_Gfx_Text_GetHeightAboveBaseline(WeUi_Gfx_Text_t* pTextObj) {
	if (!pTextObj || !WeUi_Gfx_Text_ApplySettings(pTextObj))
		return 0;

	return (u8)(Font.font_info.max_char_height + Font.font_info.y_offset);
}

u8 WeUi_Gfx_Text_GetFontHeight(const u8* pcFont) {
	if (!pcFont) {
		return 0;
	}

	u8g2_SetFont(&Font, pcFont);

	return Font.font_info.max_char_height;
}

u32 WeUi_Gfx_Text_Utf8CharLen(u8 leadChar) {
	if (leadChar < 0x80) {
		return 1;
	}
	if ((leadChar >> 5) == 0x6) {
		return 2;
	}
	if ((leadChar >> 4) == 0xE) {
		return 3;
	}
	if ((leadChar >> 3) == 0x1E) {
		return 4;
	}

	return 1;
}

u32 WeUi_Gfx_Text_Utf8FromCodePoint(u32 cp, char* pOut, u32 outSize) {
	if (!pOut || outSize == 0) {
		return 0;
	}

	pOut[0] = 0;

	if (cp > WEUI_GFX_TEXT_UTF8_MAX_CP ||
		(cp >= WEUI_GFX_TEXT_UTF8_SURR_FIRST && cp <= WEUI_GFX_TEXT_UTF8_SURR_LAST)) {
		return 0;
	}

	static const u8 leadMark[WEUI_GFX_TEXT_UTF8_MAX_BYTES + 1] = {
		[1] = WEUI_GFX_TEXT_UTF8_LEAD_1BYTE,
		[2] = WEUI_GFX_TEXT_UTF8_LEAD_2BYTE,
		[3] = WEUI_GFX_TEXT_UTF8_LEAD_3BYTE,
		[4] = WEUI_GFX_TEXT_UTF8_LEAD_4BYTE,
	};

	u32 len = 1;
	if (cp >= WEUI_GFX_TEXT_UTF8_4BYTE_FIRST) {
		len = 4;
	} else if (cp >= WEUI_GFX_TEXT_UTF8_3BYTE_FIRST) {
		len = 3;
	} else if (cp >= WEUI_GFX_TEXT_UTF8_2BYTE_FIRST) {
		len = 2;
	}

	if (outSize < len + 1) {
		return 0;
	}

	for (u32 i = len; i > 1; i--) {
		pOut[i - 1] = (char)(WEUI_GFX_TEXT_UTF8_CONT_MARK | (cp & WEUI_GFX_TEXT_UTF8_CONT_MASK));
		cp >>= WEUI_GFX_TEXT_UTF8_CONT_BITS;
	}
	pOut[0]	  = (char)(leadMark[len] | cp);
	pOut[len] = 0;

	return len;
}
