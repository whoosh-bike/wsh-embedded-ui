#ifndef __WEUI_GFX_WIDGET_TEXT_BLOCK_H
#define __WEUI_GFX_WIDGET_TEXT_BLOCK_H

#include "weui_gfx_types.h"

/**
 * Text block widget: wrapping, alignment, optional plate and border.
 *
 * Geometry is a box (x, y, w, h) relative to Style.RefPtPtr:
 *   y                 always the top of the box
 *   w == 0            box spans to the right screen edge
 *   h == 0            height follows the text, VAlign is not used
 *   h > 0             block is aligned inside the box by VAlign
 *   RefPtPtr == NULL  treated as {0, 0}
 *
 * RefPtPtr moves the whole screen: the block is laid out as without it, then shifted.
 *
 * Empty or NULL string, or no font, draws nothing and returns y unchanged. The
 * frame and the FPS belong to the scene, the widget never clears nor flushes it
 */

typedef struct {
	const WeUi_Gfx_Point_t* RefPtPtr;
	const u8* FontPtr;
	WeUi_Gfx_Color_t TextColor;
	WeUi_Gfx_Color_t BgColor;
	WeUi_Gfx_PtPos_t LineGap;
	WEUI_TEXT_ALIGN_t Align;
	WEUI_TEXT_VALIGN_t VAlign;
	bool IsWrapped;
	bool IsBgFilled;
	bool IsBordered;
} WeUi_Gfx_Widget_TextBlock_Style_t;

/**
 * @brief Fill the style with defaults: white on black, no line gap, LEFT/TOP,
 *        wrapping on, no plate and no border. Override the fields you need
 */
void WeUi_Gfx_Widget_TextBlock_StyleInit(WeUi_Gfx_Widget_TextBlock_Style_t* pStyle,
										 const WeUi_Gfx_Point_t* pcRefPt, const u8* pcFont);

/**
 * @brief Swap TextColor with BgColor and turn the plate on. Marks the selected
 *        item: a list row under the cursor, a focused button or keyboard key
 */
void WeUi_Gfx_Widget_TextBlock_StyleInvert(WeUi_Gfx_Widget_TextBlock_Style_t* pStyle);

/** @brief Height of the whole block once wrapped to the width w. Draws nothing */
WeUi_Gfx_ObjSize_t
WeUi_Gfx_Widget_TextBlock_GetHeight(const WeUi_Gfx_Widget_TextBlock_Style_t* pcStyle,
									const char* pcStr, WeUi_Gfx_ObjSize_t w);

/**
 * @brief Draw the text: wrapping, alignment on both axes, plate and border
 * @return Bottom y of the block, for chaining the next one under it
 */
WeUi_Gfx_PtPos_t WeUi_Gfx_Widget_TextBlock_Draw(const WeUi_Gfx_Widget_TextBlock_Style_t* pcStyle,
												const char* pcStr, WeUi_Gfx_PtPos_t x,
												WeUi_Gfx_PtPos_t y, WeUi_Gfx_ObjSize_t w,
												WeUi_Gfx_ObjSize_t h);

/** @brief Draw the text centered on the screen moved by pcRefPt, wrapped to its width */
void WeUi_Gfx_Widget_TextBlock_DrawScreenCentered(const WeUi_Gfx_Point_t* pcRefPt,
												  const char* pcStr, const u8* pcFont);

#endif /* __WEUI_GFX_WIDGET_TEXT_BLOCK_H */
