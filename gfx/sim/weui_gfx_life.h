#ifndef __WEUI_GFX_LIFE_H
#define __WEUI_GFX_LIFE_H

#include "weui_gfx_types.h"

/**
 * @brief Conway's Game of Life on one field the size of the panel. The field is
 *        static, WEUI_DISPLAY_WIDTH x WEUI_DISPLAY_HEIGHT: a larger panel taken at
 *        Init gets a field of that size in its top-left corner. Stepping and drawing
 *        are separate, so a scene decides how often a generation passes and draws
 *        every frame.
 */

typedef enum {
	WEUI_GFX_LIFE_CELL_DEAD = 0,
	WEUI_GFX_LIFE_CELL_ALIVE,
	WEUI_GFX_LIFE_CELL_STABLE, /* alive and left unchanged by the last step */

	WEUI_GFX_LIFE_CELL_ENUM_SIZE
} WEUI_GFX_LIFE_CELL_t;

/**
 * @brief Empty the field and size it to the panel.
 * @param wrap true: the edges join (a torus), false: outside the edge is dead.
 */
void WeUi_Gfx_Life_Init(bool wrap);

void WeUi_Gfx_Life_SetCell(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y, bool alive);
WEUI_GFX_LIFE_CELL_t WeUi_Gfx_Life_GetCell(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y);

/** @brief Fill a square at (x, y) with random cells, about half alive. Clipped to the field. */
void WeUi_Gfx_Life_SeedRandom(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y, WeUi_Gfx_ObjSize_t size);

/** @brief Kill every cell with the given probability ∈ [0.0, 1.0]. */
void WeUi_Gfx_Life_KillRandom(float ratio);

/** @brief Advance one generation. @return alive cells after the step. */
u32 WeUi_Gfx_Life_Step(void);

/** @brief Draw alive cells at pRefPt; dead cells are left as they are in the drawing frame. */
void WeUi_Gfx_Life_Draw(const WeUi_Gfx_Point_t* pRefPt, WeUi_Gfx_Color_t alive,
						WeUi_Gfx_Color_t stable);

#endif /* __WEUI_GFX_LIFE_H */
