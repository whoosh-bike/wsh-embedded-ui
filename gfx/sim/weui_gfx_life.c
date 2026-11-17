#include "weui_gfx_life.h"
#include "def_macro.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_rand.h"

#define WEUI_GFX_LIFE_CURR_MASK	  (1 << 0)
#define WEUI_GFX_LIFE_NEXT_MASK	  (1 << 1)
#define WEUI_GFX_LIFE_STABLE_MASK (1 << 2)
#define WEUI_GFX_LIFE_ALIVE		  (WEUI_GFX_LIFE_CURR_MASK | WEUI_GFX_LIFE_NEXT_MASK)
#define WEUI_GFX_LIFE_PCNT		  100

static u8 WEUI_DATA_PLACEMENT Cells[WEUI_DISPLAY_WIDTH][WEUI_DISPLAY_HEIGHT];
static WeUi_Gfx_PtPos_t Width;
static WeUi_Gfx_PtPos_t Height;
static bool Wrap;

static bool WeUi_Gfx_Life_IsInside(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y) {
	return x >= 0 && y >= 0 && x < Width && y < Height;
}

static bool WeUi_Gfx_Life_IsAlive(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y) {
	if (Wrap) {
		x = (x + Width) % Width;
		y = (y + Height) % Height;
	} else if (!WeUi_Gfx_Life_IsInside(x, y)) {
		return false;
	}

	return BIT_READ(Cells[x][y], WEUI_GFX_LIFE_CURR_MASK) != 0;
}

static u32 WeUi_Gfx_Life_CountNeighbours(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y) {
	u32 count = 0;

	for (s32 dy = -1; dy <= 1; dy++)
		for (s32 dx = -1; dx <= 1; dx++)
			if ((dx != 0 || dy != 0) && WeUi_Gfx_Life_IsAlive(x + dx, y + dy))
				count++;

	return count;
}

void WeUi_Gfx_Life_Init(bool wrap) {
	ASSERT_CHECK(WeUi_Gfx_Adapter_GetWidth() <= WEUI_DISPLAY_WIDTH);
	ASSERT_CHECK(WeUi_Gfx_Adapter_GetHeight() <= WEUI_DISPLAY_HEIGHT);

	memset(Cells, 0, sizeof(Cells));

	/* The asserts are gone in a release build, and Cells must still not be outrun */
	Width  = (WeUi_Gfx_PtPos_t)GET_MIN(WeUi_Gfx_Adapter_GetWidth(), WEUI_DISPLAY_WIDTH);
	Height = (WeUi_Gfx_PtPos_t)GET_MIN(WeUi_Gfx_Adapter_GetHeight(), WEUI_DISPLAY_HEIGHT);
	Wrap   = wrap;
}

void WeUi_Gfx_Life_SetCell(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y, bool alive) {
	if (!WeUi_Gfx_Life_IsInside(x, y))
		return;

	Cells[x][y] = alive ? WEUI_GFX_LIFE_ALIVE : 0;
}

WEUI_GFX_LIFE_CELL_t WeUi_Gfx_Life_GetCell(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y) {
	if (!WeUi_Gfx_Life_IsInside(x, y) || !BIT_READ(Cells[x][y], WEUI_GFX_LIFE_CURR_MASK))
		return WEUI_GFX_LIFE_CELL_DEAD;

	return BIT_READ(Cells[x][y], WEUI_GFX_LIFE_STABLE_MASK) ? WEUI_GFX_LIFE_CELL_STABLE
															: WEUI_GFX_LIFE_CELL_ALIVE;
}

void WeUi_Gfx_Life_SeedRandom(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y, WeUi_Gfx_ObjSize_t size) {
	for (WeUi_Gfx_PtPos_t cy = y; cy < y + (WeUi_Gfx_PtPos_t)size; cy++)
		for (WeUi_Gfx_PtPos_t cx = x; cx < x + (WeUi_Gfx_PtPos_t)size; cx++)
			WeUi_Gfx_Life_SetCell(cx, cy, WeUi_Rand_Bool());
}

void WeUi_Gfx_Life_KillRandom(float ratio) {
	s32 killPcnt = (s32)(WEUI_GFX_LIFE_PCNT * ratio);

	for (WeUi_Gfx_PtPos_t x = 0; x < Width; x++)
		for (WeUi_Gfx_PtPos_t y = 0; y < Height; y++)
			if (WeUi_Rand_Between(0, WEUI_GFX_LIFE_PCNT - 1) < killPcnt)
				Cells[x][y] = 0;
}

u32 WeUi_Gfx_Life_Step(void) {
	/* Two passes: every cell reads its neighbours' current state before any of
	 * them moves on. Rules: <2 or >3 neighbours dies, 3 is born, 2 keeps its state */
	for (WeUi_Gfx_PtPos_t x = 0; x < Width; x++) {
		for (WeUi_Gfx_PtPos_t y = 0; y < Height; y++) {
			BIT_CLEAR(Cells[x][y], WEUI_GFX_LIFE_STABLE_MASK);

			u32 neighbours = WeUi_Gfx_Life_CountNeighbours(x, y);
			if (neighbours < 2 || neighbours > 3)
				BIT_CLEAR(Cells[x][y], WEUI_GFX_LIFE_NEXT_MASK);
			else if (neighbours == 3)
				BIT_SET(Cells[x][y], WEUI_GFX_LIFE_NEXT_MASK);
			else
				BIT_SET(Cells[x][y], WEUI_GFX_LIFE_STABLE_MASK);
		}
	}

	u32 alive = 0;
	for (WeUi_Gfx_PtPos_t x = 0; x < Width; x++) {
		for (WeUi_Gfx_PtPos_t y = 0; y < Height; y++) {
			if (BIT_READ(Cells[x][y], WEUI_GFX_LIFE_NEXT_MASK)) {
				Cells[x][y] |= WEUI_GFX_LIFE_ALIVE;
				alive++;
			} else {
				Cells[x][y] = 0;
			}
		}
	}

	return alive;
}

void WeUi_Gfx_Life_Draw(const WeUi_Gfx_Point_t* pRefPt, WeUi_Gfx_Color_t alive,
						WeUi_Gfx_Color_t stable) {
	for (WeUi_Gfx_PtPos_t x = 0; x < Width; x++) {
		for (WeUi_Gfx_PtPos_t y = 0; y < Height; y++) {
			WEUI_GFX_LIFE_CELL_t cell = WeUi_Gfx_Life_GetCell(x, y);
			if (cell == WEUI_GFX_LIFE_CELL_DEAD)
				continue;

			WeUi_Gfx_ColorPoint_t cpt = {
				.Pt.X  = pRefPt->X + x,
				.Pt.Y  = pRefPt->Y + y,
				.Color = (cell == WEUI_GFX_LIFE_CELL_STABLE) ? stable : alive,
			};
			WeUi_Gfx_Point_Draw(&cpt);
		}
	}
}
