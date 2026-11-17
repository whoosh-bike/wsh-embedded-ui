#ifndef __WEUI_GFX_EFFECT_COMPOSE_H
#define __WEUI_GFX_EFFECT_COMPOSE_H

/**
 * @file weui_gfx_effect_compose.h
 * @brief What a transition's compose works with. Private to effects/, not API.
 *
 * The old picture is the outgoing one, parked in the aux frame for the whole run.
 * The new picture is the incoming one, in the drawing frame.
 *
 * A row compose (WeUi_Gfx_Effect_RowCb_t) writes one row of the frame from both pictures into
 * WeUi_Gfx_Effect_Out, through the Row* helpers below; the runner sends it to the panel in a band
 * or puts it back over the drawing frame. A frame compose works on the drawing frame in place:
 * it puts the old picture back over the new one wherever the old still shows.
 *
 * Pixels are moved raw: SetPixel would put the driver brightness on them a second time.
 */

#include <string.h>

#include "def_macro.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_effect_types.h"
#include "weui_gfx_transform.h"
#include "weui_rand.h"

/** @brief Smoothstep: sets off gently and settles the same way - the default ease */
static inline float WeUi_Gfx_Effect_Smooth(float progress) {
	return progress * progress * (3.0f - 2.0f * progress);
}

/**
 * @brief The drawing and aux frames as memory, while a transition plays on a driver that keeps
 *        them so (WEUI_DISPLAY_FRAME_MEM): the pixels below go straight to them then, a run of
 *        them by memcpy, and through the adapter a pixel at a time otherwise. Set by the runner.
 */
typedef struct {
	bool On;
	WeUi_Gfx_FrameMem_t New; /* the drawing frame */
	WeUi_Gfx_FrameMem_t Old; /* the aux frame: the same pixels as the drawing one */
} WeUi_Gfx_Effect_Mem_t;

extern WeUi_Gfx_Effect_Mem_t WeUi_Gfx_Effect_Mem;

static inline u32 WeUi_Gfx_Effect_At(const WeUi_Gfx_FrameMem_t* pcMem, WeUi_Gfx_PtPos_t x,
									 WeUi_Gfx_PtPos_t y) {
	return (u32)y * pcMem->Stride + (u32)x;
}

static inline u8* WeUi_Gfx_Effect_Byte(const WeUi_Gfx_FrameMem_t* pcMem, WeUi_Gfx_PtPos_t x,
									   WeUi_Gfx_PtPos_t y) {
	return (u8*)pcMem->pBuf + WeUi_Gfx_Effect_At(pcMem, x, y) * WeUi_Gfx_Adapter_FramePxBytes(pcMem);
}

static inline void WeUi_Gfx_Effect_CopyPx(u8* pDst, const u8* pcSrc, u32 bytes) {
	if (bytes == sizeof(u16))
		*(u16*)pDst = *(const u16*)pcSrc;
	else
		*(WeUi_Gfx_ColorRaw_t*)pDst = *(const WeUi_Gfx_ColorRaw_t*)pcSrc;
}

/** @brief The incoming picture's pixel, raw */
static inline WeUi_Gfx_ColorRaw_t WeUi_Gfx_Effect_GetNew(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y) {
	if (WeUi_Gfx_Effect_Mem.On)
		return WeUi_Gfx_Adapter_FrameGet(&WeUi_Gfx_Effect_Mem.New,
										 WeUi_Gfx_Effect_At(&WeUi_Gfx_Effect_Mem.New, x, y));
	WeUi_Gfx_Point_t pt = {.X = x, .Y = y};
	return WeUi_Gfx_Adapter_GetPixelRaw(&pt);
}

/** @brief The old picture's pixel, raw */
static inline WeUi_Gfx_ColorRaw_t WeUi_Gfx_Effect_GetOld(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y) {
	if (WeUi_Gfx_Effect_Mem.On)
		return WeUi_Gfx_Adapter_FrameGet(&WeUi_Gfx_Effect_Mem.Old,
										 WeUi_Gfx_Effect_At(&WeUi_Gfx_Effect_Mem.Old, x, y));
	WeUi_Gfx_Point_t pt = {.X = x, .Y = y};
	return WeUi_Gfx_Adapter_GetPixelRaw_Aux(&pt);
}

/** @brief A pixel of the frame being composed, set raw */
static inline void WeUi_Gfx_Effect_SetNew(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y,
										  WeUi_Gfx_ColorRaw_t raw) {
	if (WeUi_Gfx_Effect_Mem.On) {
		WeUi_Gfx_Adapter_FrameSet(&WeUi_Gfx_Effect_Mem.New,
								  WeUi_Gfx_Effect_At(&WeUi_Gfx_Effect_Mem.New, x, y), raw);
		return;
	}
	WeUi_Gfx_Point_t pt = {.X = x, .Y = y};
	WeUi_Gfx_Adapter_SetPixelRaw(&pt, raw);
}

/** @brief The old picture's pixel (sx, sy) at (x, y): (x, y) itself where it stands still */
static inline void WeUi_Gfx_Effect_PutOld(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y,
										  WeUi_Gfx_PtPos_t sx, WeUi_Gfx_PtPos_t sy) {
	if (WeUi_Gfx_Effect_Mem.On) {
		WeUi_Gfx_Effect_CopyPx(WeUi_Gfx_Effect_Byte(&WeUi_Gfx_Effect_Mem.New, x, y),
							   WeUi_Gfx_Effect_Byte(&WeUi_Gfx_Effect_Mem.Old, sx, sy),
							   WeUi_Gfx_Adapter_FramePxBytes(&WeUi_Gfx_Effect_Mem.New));
		return;
	}
	WeUi_Gfx_Effect_SetNew(x, y, WeUi_Gfx_Effect_GetOld(sx, sy));
}

/** @brief The incoming picture's pixel (sx, sy) moved to (x, y), within its own frame */
static inline void WeUi_Gfx_Effect_MoveNew(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y,
										   WeUi_Gfx_PtPos_t sx, WeUi_Gfx_PtPos_t sy) {
	if (WeUi_Gfx_Effect_Mem.On) {
		WeUi_Gfx_Effect_CopyPx(WeUi_Gfx_Effect_Byte(&WeUi_Gfx_Effect_Mem.New, x, y),
							   WeUi_Gfx_Effect_Byte(&WeUi_Gfx_Effect_Mem.New, sx, sy),
							   WeUi_Gfx_Adapter_FramePxBytes(&WeUi_Gfx_Effect_Mem.New));
		return;
	}
	WeUi_Gfx_Effect_SetNew(x, y, WeUi_Gfx_Effect_GetNew(sx, sy));
}

/** @brief n of the old picture's pixels from (sx, sy) on along the row, at (x, y) on */
static inline void WeUi_Gfx_Effect_PutOldRun(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y,
											 WeUi_Gfx_PtPos_t sx, WeUi_Gfx_PtPos_t sy,
											 WeUi_Gfx_PtPos_t n) {
	if (n <= 0)
		return;
	if (WeUi_Gfx_Effect_Mem.On) {
		memcpy(WeUi_Gfx_Effect_Byte(&WeUi_Gfx_Effect_Mem.New, x, y),
			   WeUi_Gfx_Effect_Byte(&WeUi_Gfx_Effect_Mem.Old, sx, sy),
			   (size_t)n * WeUi_Gfx_Adapter_FramePxBytes(&WeUi_Gfx_Effect_Mem.New));
		return;
	}
	for (WeUi_Gfx_PtPos_t i = 0; i < n; i++)
		WeUi_Gfx_Effect_PutOld(x + i, y, sx + i, sy);
}

/** @brief n pixels from (x, y) on along the row set to one raw colour */
static inline void WeUi_Gfx_Effect_SetNewRun(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y,
											 WeUi_Gfx_PtPos_t n, WeUi_Gfx_ColorRaw_t raw) {
	for (WeUi_Gfx_PtPos_t i = 0; i < n; i++)
		WeUi_Gfx_Effect_SetNew(x + i, y, raw);
}

/** @brief The whole old picture back over the new one */
static inline void WeUi_Gfx_Effect_PutOldFrame(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)pcCtx->Height; y++)
		WeUi_Gfx_Effect_PutOldRun(0, y, 0, y, (WeUi_Gfx_PtPos_t)pcCtx->Width);
}

/**
 * @brief The row a row compose writes, from x = 0: a row of a band on its way to the panel, in
 *        the band's pixels, or a row of raw ones the runner puts back over the drawing frame.
 *        Set by the runner.
 */
extern WeUi_Gfx_FrameMem_t WeUi_Gfx_Effect_Out;

static inline void WeUi_Gfx_Effect_RowSet(WeUi_Gfx_PtPos_t x, WeUi_Gfx_ColorRaw_t raw) {
	WeUi_Gfx_Adapter_FrameSet(&WeUi_Gfx_Effect_Out, (u32)x, raw);
}

/** @brief n pixels of a frame as memory from (sx, sy) on along its row, at x on in the row */
static inline void WeUi_Gfx_Effect_RowCopy(const WeUi_Gfx_FrameMem_t* pcSrc, WeUi_Gfx_PtPos_t x,
										   WeUi_Gfx_PtPos_t sx, WeUi_Gfx_PtPos_t sy,
										   WeUi_Gfx_PtPos_t n) {
	if (pcSrc->Px == WeUi_Gfx_Effect_Out.Px) {
		memcpy(WeUi_Gfx_Effect_Byte(&WeUi_Gfx_Effect_Out, x, 0), WeUi_Gfx_Effect_Byte(pcSrc, sx, sy),
			   (size_t)n * WeUi_Gfx_Adapter_FramePxBytes(pcSrc));
		return;
	}
	u32 at = WeUi_Gfx_Effect_At(pcSrc, sx, sy);
	for (WeUi_Gfx_PtPos_t i = 0; i < n; i++)
		WeUi_Gfx_Effect_RowSet(x + i, WeUi_Gfx_Adapter_FrameGet(pcSrc, at + (u32)i));
}

/** @brief n of the old picture's pixels from (sx, sy) on along its row, at x on in the row */
static inline void WeUi_Gfx_Effect_RowOld(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t sx,
										  WeUi_Gfx_PtPos_t sy, WeUi_Gfx_PtPos_t n) {
	if (n <= 0)
		return;
	if (WeUi_Gfx_Effect_Mem.On) {
		WeUi_Gfx_Effect_RowCopy(&WeUi_Gfx_Effect_Mem.Old, x, sx, sy, n);
		return;
	}
	for (WeUi_Gfx_PtPos_t i = 0; i < n; i++)
		WeUi_Gfx_Effect_RowSet(x + i, WeUi_Gfx_Effect_GetOld(sx + i, sy));
}

/** @brief n of the new picture's pixels from (sx, sy) on along its row, at x on in the row */
static inline void WeUi_Gfx_Effect_RowNew(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t sx,
										  WeUi_Gfx_PtPos_t sy, WeUi_Gfx_PtPos_t n) {
	if (n <= 0)
		return;
	if (WeUi_Gfx_Effect_Mem.On) {
		WeUi_Gfx_Effect_RowCopy(&WeUi_Gfx_Effect_Mem.New, x, sx, sy, n);
		return;
	}
	for (WeUi_Gfx_PtPos_t i = 0; i < n; i++)
		WeUi_Gfx_Effect_RowSet(x + i, WeUi_Gfx_Effect_GetNew(sx + i, sy));
}

/** @brief Pixel (sx, sy) of a frame as memory at x in the row: a pixel's worth of RowCopy() */
static inline void WeUi_Gfx_Effect_RowPx(const WeUi_Gfx_FrameMem_t* pcSrc, WeUi_Gfx_PtPos_t x,
										 WeUi_Gfx_PtPos_t sx, WeUi_Gfx_PtPos_t sy) {
	u32 at = WeUi_Gfx_Effect_At(pcSrc, sx, sy);
	if (pcSrc->Px != WeUi_Gfx_Effect_Out.Px) {
		WeUi_Gfx_Effect_RowSet(x, WeUi_Gfx_Adapter_FrameGet(pcSrc, at));
		return;
	}
	if (pcSrc->Px == WEUI_GFX_FRAME_PX_RAW)
		((WeUi_Gfx_ColorRaw_t*)WeUi_Gfx_Effect_Out.pBuf)[x] =
			((const WeUi_Gfx_ColorRaw_t*)pcSrc->pBuf)[at];
	else
		((u16*)WeUi_Gfx_Effect_Out.pBuf)[x] = ((const u16*)pcSrc->pBuf)[at];
}

/** @brief The old picture's pixel (sx, sy) at x in the row */
static inline void WeUi_Gfx_Effect_RowOldPx(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t sx,
											WeUi_Gfx_PtPos_t sy) {
	if (WeUi_Gfx_Effect_Mem.On)
		WeUi_Gfx_Effect_RowPx(&WeUi_Gfx_Effect_Mem.Old, x, sx, sy);
	else
		WeUi_Gfx_Effect_RowSet(x, WeUi_Gfx_Effect_GetOld(sx, sy));
}

/** @brief The new picture's pixel (sx, sy) at x in the row */
static inline void WeUi_Gfx_Effect_RowNewPx(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t sx,
											WeUi_Gfx_PtPos_t sy) {
	if (WeUi_Gfx_Effect_Mem.On)
		WeUi_Gfx_Effect_RowPx(&WeUi_Gfx_Effect_Mem.New, x, sx, sy);
	else
		WeUi_Gfx_Effect_RowSet(x, WeUi_Gfx_Effect_GetNew(sx, sy));
}

/** @brief n pixels from x on in the row set to one raw colour */
static inline void WeUi_Gfx_Effect_RowFill(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t n,
										   WeUi_Gfx_ColorRaw_t raw) {
	if (n <= 0)
		return;
	/* Turned into the row's pixels once, then copied on */
	WeUi_Gfx_Effect_RowSet(x, raw);
	if (WeUi_Gfx_Effect_Out.Px == WEUI_GFX_FRAME_PX_RAW) {
		for (WeUi_Gfx_PtPos_t i = 1; i < n; i++)
			WeUi_Gfx_Effect_RowSet(x + i, raw);
		return;
	}
	u16* pRow = (u16*)WeUi_Gfx_Effect_Out.pBuf;
	for (WeUi_Gfx_PtPos_t i = 1; i < n; i++)
		pRow[x + i] = pRow[x];
}

/*
 * 5-6-5 pixels mixed all three channels at once: spread apart with gaps between them, a channel
 * takes its borrow into the gap below and none leaks into the next. A weight is 0..32. What a
 * band of 565 from frames of 565 gets, where the raw colours' float mix would cost a frame its
 * pace; the picture differs from the raw mix's by the weight's steps.
 */
#define WEUI_GFX_EFFECT_565_SPREAD 0x07E0F81Fu
#define WEUI_GFX_EFFECT_565_WEIGHT 32.0f

/** @brief Both pictures and the row in the same 5-6-5: mixed there, never through raw colours */
static inline bool WeUi_Gfx_Effect_Out565(void) {
	return WeUi_Gfx_Effect_Out.Px != WEUI_GFX_FRAME_PX_RAW && WeUi_Gfx_Effect_Mem.On &&
		   WeUi_Gfx_Effect_Mem.Old.Px == WeUi_Gfx_Effect_Out.Px;
}

/**
 * @brief Row y of both pictures and the row being composed, all in the same 5-6-5: for the
 *        loops that go a pixel at a time. Locals, not the globals, so that a store to the row
 *        does not make the loop read them all again.
 */
typedef struct {
	u16* pOut;
	const u16* pcOld;
	const u16* pcNew;
	bool Be; /* high byte first */
} WeUi_Gfx_Effect_Rows565_t;

/** @brief False unless Out565(): the Row* helpers are the way then */
static inline bool WeUi_Gfx_Effect_Rows565(WeUi_Gfx_PtPos_t y, WeUi_Gfx_Effect_Rows565_t* pRows) {
	if (!WeUi_Gfx_Effect_Out565())
		return false;
	const WeUi_Gfx_Effect_Mem_t* pcM = &WeUi_Gfx_Effect_Mem;
	pRows->pOut						 = (u16*)WeUi_Gfx_Effect_Out.pBuf;
	pRows->pcOld = (const u16*)pcM->Old.pBuf + WeUi_Gfx_Effect_At(&pcM->Old, 0, y);
	pRows->pcNew = (const u16*)pcM->New.pBuf + WeUi_Gfx_Effect_At(&pcM->New, 0, y);
	pRows->Be	 = WeUi_Gfx_Effect_Out.Px == WEUI_GFX_FRAME_PX_RGB565_BE;
	return true;
}

static inline u32 WeUi_Gfx_Effect_Weight565(float share) {
	return (u32)(CUT_NORM_FLOAT(share) * WEUI_GFX_EFFECT_565_WEIGHT + 0.5f);
}

static inline u32 WeUi_Gfx_Effect_Spread565(u16 px, bool be) {
	u32 c = be ? __builtin_bswap16(px) : px;
	return (c | c << 16) & WEUI_GFX_EFFECT_565_SPREAD;
}

static inline u16 WeUi_Gfx_Effect_Fold565(u32 spread, bool be) {
	u16 px = (u16)(spread | spread >> 16);
	return be ? __builtin_bswap16(px) : px;
}

/** @brief from + (to - from) * weight / 32, spread pixels */
static inline u32 WeUi_Gfx_Effect_Mix565(u32 from, u32 to, u32 weight) {
	return ((((to - from) * weight) >> 5) + from) & WEUI_GFX_EFFECT_565_SPREAD;
}

/** @brief `raw` darkened to `keep` of itself, as Frame_Fade() darkens a pixel */
static inline WeUi_Gfx_ColorRaw_t WeUi_Gfx_Effect_Faded(WeUi_Gfx_ColorRaw_t raw, float keep) {
	WeUi_Gfx_Color_t color = WeUi_Gfx_Transform_Bytes2Color(raw);
	color.R				   = (WeUi_Gfx_ColorChannel_t)(color.R * keep);
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	color.G = (WeUi_Gfx_ColorChannel_t)(color.G * keep);
	color.B = (WeUi_Gfx_ColorChannel_t)(color.B * keep);
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */
	return WeUi_Gfx_Transform_Color2Bytes(color);
}

/** @brief n pixels of the row from x on darkened to `keep` in place: only while the row holds
 *         the pictures' own 5-6-5 (Out565()), which darkened there is what they would give */
static inline void WeUi_Gfx_Effect_RowFade565(WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t n, float keep) {
	u32 weight = WeUi_Gfx_Effect_Weight565(keep);
	u16* pRow  = (u16*)WeUi_Gfx_Effect_Out.pBuf;
	bool be	   = WeUi_Gfx_Effect_Out.Px == WEUI_GFX_FRAME_PX_RGB565_BE;
	for (WeUi_Gfx_PtPos_t i = x; i < x + n; i++)
		pRow[i] = WeUi_Gfx_Effect_Fold565(
			WeUi_Gfx_Effect_Mix565(0, WeUi_Gfx_Effect_Spread565(pRow[i], be), weight), be);
}

/** @brief n of the old picture's pixels, or the new one's, from (sx, sy) on along its row,
 *         darkened to `keep`, at x on in the row */
static inline void WeUi_Gfx_Effect_RowFaded(bool fromOld, WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t sx,
											WeUi_Gfx_PtPos_t sy, WeUi_Gfx_PtPos_t n, float keep) {
	if (WeUi_Gfx_Effect_Out565()) {
		WeUi_Gfx_Effect_RowCopy(fromOld ? &WeUi_Gfx_Effect_Mem.Old : &WeUi_Gfx_Effect_Mem.New, x, sx,
								sy, n);
		WeUi_Gfx_Effect_RowFade565(x, n, keep);
		return;
	}
	for (WeUi_Gfx_PtPos_t i = 0; i < n; i++) {
		WeUi_Gfx_ColorRaw_t raw =
			fromOld ? WeUi_Gfx_Effect_GetOld(sx + i, sy) : WeUi_Gfx_Effect_GetNew(sx + i, sy);
		WeUi_Gfx_Effect_RowSet(x + i, WeUi_Gfx_Effect_Faded(raw, keep));
	}
}

/**
 * @brief A dark pixel, raw. The raw domain has no colour API, but the minimum channel is
 *        a fixed point of the driver's brightness LUT, so the two agree here.
 */
static inline WeUi_Gfx_ColorRaw_t WeUi_Gfx_Effect_Blank(void) {
	return WeUi_Gfx_Transform_FillBytesFromChannel(WeUi_Gfx_Adapter_GetColor_Min());
}

/** @brief `raw` lifted towards white by `shine` in [0; 1] */
static inline WeUi_Gfx_ColorRaw_t WeUi_Gfx_Effect_Lighten(WeUi_Gfx_ColorRaw_t raw, float shine) {
	WeUi_Gfx_Color_t color = WeUi_Gfx_Transform_Bytes2Color(raw);
	float max			   = (float)WeUi_Gfx_Adapter_GetColor_Max();

	color.R = (WeUi_Gfx_ColorChannel_t)(color.R + (max - color.R) * shine);
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	color.G = (WeUi_Gfx_ColorChannel_t)(color.G + (max - color.G) * shine);
	color.B = (WeUi_Gfx_ColorChannel_t)(color.B + (max - color.B) * shine);
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

	return WeUi_Gfx_Transform_Color2Bytes(color);
}

/** @brief The moment cell (a, b) takes its turn, in [0; 1): scattered, the same every run */
static inline float WeUi_Gfx_Effect_Turn(u32 a, u32 b) {
	return (float)(WeUi_Rand_Hash(a, b, 0) & 0xFFFFu) / 65536.0f;
}

/**
 * @brief How far column `x` is along its own part of the run, [0; 1]. Columns
 *        `columnPx` wide each start at a turn of their own within the first `spread`.
 */
static inline float WeUi_Gfx_Effect_ColumnRun(const WeUi_Gfx_Effect_Ctx_t* pcCtx,
											  WeUi_Gfx_PtPos_t x, u8 columnPx, float spread) {
	float start = spread * WeUi_Gfx_Effect_Turn((u32)x / GET_MAX(columnPx, 1u), 0);
	return CUT_NORM_FLOAT((pcCtx->Progress - start) / (1.0f - spread));
}

#endif /* __WEUI_GFX_EFFECT_COMPOSE_H */
