#ifndef __WEUI_GFX_TEXT_H
#define __WEUI_GFX_TEXT_H

#include "u8g2.h"
#include "weui_gfx_types.h"

/*
 * U8G2 font naming convention: u8g2_font_<name>_<S><C>
 *
 * <S> - Spacing:
 *   t = proportional (variable width)
 *   m = monospace (fixed width)
 *
 * <C> - Character set:
 *   f = full        (glyphs 32-255)
 *   r = reduced     (glyphs 32-127, basic Latin)
 *   n = numbers     (glyphs 48-57, digits and basic punctuation)
 *   e = extended    (glyphs 32-701)
 *
 * Special suffixes:
 *   _t_cyrillic  = proportional, Cyrillic charset
 *   _t_symbols   = proportional, symbol charset
 *   _t_hebrew    = proportional, Hebrew charset
 *
 * Font name modifiers:
 *   B = Bold       (e.g. u8g2_font_7x13B_tr)
 *   O = Oblique    (e.g. u8g2_font_7x13O_tr)
 *
 * Examples:
 *   u8g2_font_8x13_t_cyrillic  - 8x13 proportional, Cyrillic
 *   u8g2_font_fub25_tr         - FreeUniversal Bold 25px, proportional, reduced Latin
 *   u8g2_font_fur14_tf         - FreeUniversal Regular 14px, proportional, full
 *   u8g2_font_open_iconic_weather_4x_t - Open Iconic weather icons, 4x scale
 */

/** @brief Longest UTF-8 sequence, and the buffer size that also holds the terminator. */
#define WEUI_GFX_TEXT_UTF8_MAX_BYTES 4U
#define WEUI_GFX_TEXT_UTF8_BUF_SIZE	 (WEUI_GFX_TEXT_UTF8_MAX_BYTES + 1U)

/** @brief Initialize u8g2 font rendering context. Call once before any WeUi_Gfx_Text_Draw calls. */
void WeUi_Gfx_Text_Init(void);

/** @brief Zero-initialize WeUi_Gfx_Text_t and set default background mode and rotation. */
void WeUi_Gfx_Text_InitStruct(WeUi_Gfx_Text_t* pTextObj);

/**
 * @brief Render text string at the reference point using u8g2 glyph scanlines.
 * @param pRefPt  Top-left corner of the text bounding box
 * @param pTextObj  Text descriptor (string, font, color, background mode, rotation)
 * @return RET_STATE_OK on success
 */
RET_STATE_t WeUi_Gfx_Text_Draw(WeUi_Gfx_Point_t* pRefPt, WeUi_Gfx_Text_t* pTextObj);

/** @brief Return rendered pixel width of the text string in pTextObj (does not draw). */
u32 WeUi_Gfx_Text_GetPixLen(WeUi_Gfx_Text_t* pTextObj);

/**
 * @brief Return rendered pixel width of the first len bytes of pcStr, so that a
 *        substring is measured without being copied out and zero-terminated.
 *        pTextObj supplies the font and the draw mode, its Str is not used
 */
u32 WeUi_Gfx_Text_GetPixLenN(WeUi_Gfx_Text_t* pTextObj, const char* pcStr, u32 len);

/**
 * @brief Return the length in bytes of the longest prefix of pcStr[0..len) that
 *        renders within maxWidth. Never splits a UTF-8 sequence, so the result
 *        is a valid substring; 0 means not even the first character fits
 */
u32 WeUi_Gfx_Text_FitLen(WeUi_Gfx_Text_t* pTextObj, const char* pcStr, u32 len,
						 WeUi_Gfx_ObjSize_t maxWidth);

/**
 * @brief Render the first len bytes of pcStr aligned inside a band of the given
 *        width. pRefPt is the left end of the band on the text baseline, as in
 *        WeUi_Gfx_Text_Draw. Text wider than the band is drawn from the left
 * @return RET_STATE_OK on success
 */
RET_STATE_t WeUi_Gfx_Text_DrawAligned(WeUi_Gfx_Point_t* pRefPt, WeUi_Gfx_Text_t* pTextObj,
									  const char* pcStr, u32 len, WeUi_Gfx_ObjSize_t width,
									  WEUI_TEXT_ALIGN_t align);

/** @brief Return the glyph box height in pixels for the font set in pTextObj. */
u8 WeUi_Gfx_Text_GetHeight(WeUi_Gfx_Text_t* pTextObj);

/**
 * @brief Return how many rows the font may paint above and including the
 *        baseline, for the font set in pTextObj. This is what a draw fits into
 *        vertically: WeUi_Gfx_Text_GetHeight() counts the descender rows below
 *        the baseline as well, and rejects fonts that would have fit.
 */
u8 WeUi_Gfx_Text_GetHeightAboveBaseline(WeUi_Gfx_Text_t* pTextObj);

/**
 * @brief Return the glyph box height in pixels for pcFont, without a text object.
 *        Height depends on the font alone, so this answers "how tall is one line"
 *        for layout done before any string is known
 */
u8 WeUi_Gfx_Text_GetFontHeight(const u8* pcFont);

/** @brief Return the UTF-8 sequence length (1-4 bytes) encoded in the lead byte. */
u32 WeUi_Gfx_Text_Utf8CharLen(u8 leadChar);

/**
 * @brief Encode one Unicode code point as a zero-terminated UTF-8 string, so that a
 *        glyph known by its code point can be passed to the draw and measure calls,
 *        which all take strings. Icon fonts keep their glyphs outside the ASCII
 *        range, where a plain character literal cannot reach them
 * @param cp  Code point to encode, U+0000..U+10FFFF and not a surrogate
 * @param pOut  Output buffer, WEUI_GFX_TEXT_UTF8_BUF_SIZE bytes always fit any code point
 * @param outSize  Size of pOut in bytes, the terminator included
 * @return Sequence length in bytes without the terminator, or 0 if cp is not a code
 *         point or does not fit in pOut, in which case pOut is left empty
 */
u32 WeUi_Gfx_Text_Utf8FromCodePoint(u32 cp, char* pOut, u32 outSize);

#endif /* __WEUI_GFX_TEXT_H */
