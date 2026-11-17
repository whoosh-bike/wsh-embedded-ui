#include "weui_gfx_fill.h"

#include "weui_gfx_adapter.h"
#include "weui_gfx_transform.h"
#include <float.h>

#define WEUI_GFX_FILL_CHANNEL_BITS 8
#define WEUI_GFX_FILL_CHANNEL_MASK 0xFFu
#define WEUI_GFX_FILL_CHANNELS_NUM 4 /* bytes of a stored pixel, 0x00BBGGRR or with alpha */
/* 0..255 onto 0..256, so full coverage or opacity multiplies by exactly 1 */
#define WEUI_GFX_FILL_FULL_ROUND   7
/* Colours of a gradient, taken through the driver once per fill */
#define WEUI_GFX_FILL_LUT_SIZE	   256
#define WEUI_GFX_FILL_LUT_LAST	   (WEUI_GFX_FILL_LUT_SIZE - 1)
/* A solid paint's colour has a slot of its own, past the gradients': theirs stay for their next
 * fills */
#define WEUI_GFX_FILL_LUT_SOLID	   (WEUI_GFX_FILL_LUTS * WEUI_GFX_FILL_LUT_SIZE)
#define WEUI_GFX_FILL_LUT_SLOTS	   (WEUI_GFX_FILL_LUT_SOLID + 1)
/* Stops of a gradient whose colours for a buffer are kept; one with more is worked out anew */
#define WEUI_GFX_FILL_KEY_STOPS	   8
#define WEUI_GFX_FILL_PIXEL_MID	   0.5f
/* RGB565: the top 5, 6 and 5 bits of each channel */
#define WEUI_GFX_FILL_565_R_MASK   0xF8u
#define WEUI_GFX_FILL_565_G_MASK   0xFCu
#define WEUI_GFX_FILL_565_R_SHIFT  8
#define WEUI_GFX_FILL_565_G_SHIFT  3
#define WEUI_GFX_FILL_565_B_SHIFT  3
#define WEUI_GFX_FILL_5_REPEAT	   5
#define WEUI_GFX_FILL_6_REPEAT	   6
#define WEUI_GFX_FILL_G_AT		   8
#define WEUI_GFX_FILL_B_AT		   16
#define WEUI_GFX_FILL_PAIR_BITS	   16 /* two pixels to a 32-bit write */
#define WEUI_GFX_FILL_PAIR_ALIGN   (sizeof(u32) - 1u)
/* Masks a span into a buffer reads a row at a time; a paint with more goes pixel by pixel */
#define WEUI_GFX_FILL_SPAN_MASKS   4
/* No row worked out yet: rows on the panel are never negative */
#define WEUI_GFX_FILL_ROW_NONE	   (-1)

typedef struct {
	const u8* pcRow; /**< the mask's row, its first pixel at X0 */
	s32 X0;
	s32 X1;
	bool Out;
} WeUi_Gfx_FillMaskRow_t;

/* A stored colour's channels apart, for the pixels of a buffer to blend with */
typedef struct {
	u32 R;
	u32 G;
	u32 B;
} WeUi_Gfx_FillRgb_t;

typedef struct {
	const WeUi_Gfx_Paint_t* pcPaint;
	WeUi_Gfx_Affine_t Inv; /**< panel onto the paint's coordinates */
	/* LINEAR: offset = Gx * x + Gy * y + G0 in the paint's coordinates */
	float Gx;
	float Gy;
	float G0;
	float InvRadius; /**< RADIAL */
	float DtDx;		 /**< LINEAR: how the offset changes a pixel to the right */
	u32 Opacity;	 /**< 0..256 */
	bool Probed;
	u32 LutAt;						   /**< where a gradient's colours start in the table */
	const WeUi_Gfx_Target_t* pcTarget; /**< NULL: the drawing frame */
	u8 RasterId;					   /**< the raster set of pcTarget's, 0 for the drawing frame */
	bool Plain; /**< one colour, opaque, no masks: an edgeless run is written as it is */
	bool Fixed; /**< the colour does not change along a row: solid, or linear down the rows */
	/* What the spans of one row of a buffer share, worked out with its first span */
	s32 RowY;	   /**< WEUI_GFX_FILL_ROW_NONE before the first */
	bool RowShown; /**< every mask kept inside reaches the row */
	s32 RowX0;	   /**< where they all reach, on the panel */
	s32 RowX1;
	WeUi_Gfx_FillMaskRow_t RowMasks[WEUI_GFX_FILL_SPAN_MASKS];
	u32 RowMasksNum;
	u16* pRowPx;				/**< the row's pixel at the target's X0 */
	u16 RowWhole;				/**< Fixed: the row's colour as a pixel */
	WeUi_Gfx_FillRgb_t RowOver; /**< and as channels, to blend with */
} WeUi_Gfx_FillCtx_t;

/* One fill at a time, as the rasteriser. As the driver stores the colours for the frame, as
 * 0x00BBGGRR for a buffer. Two sets: two cores may fill two bands of one frame at once */
static WeUi_Gfx_ColorRaw_t WEUI_DATA_PLACEMENT Lut[WEUI_GFX_RASTER_SETS][WEUI_GFX_FILL_LUT_SLOTS];
/* The same colours as a buffer's pixels, for a pixel the paint covers whole */
static u16 WEUI_DATA_PLACEMENT Lut565[WEUI_GFX_RASTER_SETS][WEUI_GFX_FILL_LUT_SLOTS];

/* The gradients the tables hold a buffer's colours of: a shape drawn band by band, or two shapes
 * of one gradient, need them worked out once */
typedef struct {
	bool Valid;
	WEUI_GFX_TARGET_t Format;
	u8 StopsNum;
	float Offsets[WEUI_GFX_FILL_KEY_STOPS];
	WeUi_Gfx_ColorRaw_t Colors[WEUI_GFX_FILL_KEY_STOPS];
} WeUi_Gfx_FillLutKey_t;

static WeUi_Gfx_FillLutKey_t LutKeys[WEUI_GFX_RASTER_SETS][WEUI_GFX_FILL_LUTS];
/* The table to give up next, round the tables in turn */
static u32 LutNext[WEUI_GFX_RASTER_SETS];

WeUi_Gfx_ClipRect_t WeUi_Gfx_Fill_PanelClip(void) {
	return (WeUi_Gfx_ClipRect_t){
		.X0 = WeUi_Gfx_Adapter_GetX_Min(),
		.Y0 = WeUi_Gfx_Adapter_GetY_Min(),
		.X1 = (WeUi_Gfx_PtPos_t)(WeUi_Gfx_Adapter_GetX_Max() + 1),
		.Y1 = (WeUi_Gfx_PtPos_t)(WeUi_Gfx_Adapter_GetY_Max() + 1),
	};
}

WeUi_Gfx_Paint_t WeUi_Gfx_Paint_Solid(WeUi_Gfx_Color_t color) {
	return (WeUi_Gfx_Paint_t){
		.Type	 = WEUI_GFX_PAINT_SOLID,
		.Color	 = color,
		.Opacity = WEUI_GFX_PAINT_OPAQUE,
	};
}

static u32 WeUi_Gfx_Fill_ToMul(u32 v) {
	return v + (v >> WEUI_GFX_FILL_FULL_ROUND);
}

/** @brief under + (over - under) * share, byte by byte; share 0..255 */
static WeUi_Gfx_ColorRaw_t WeUi_Gfx_Fill_Blend(WeUi_Gfx_ColorRaw_t under, WeUi_Gfx_ColorRaw_t over,
											   u32 share) {
	s32 a					= (s32)WeUi_Gfx_Fill_ToMul(share);
	WeUi_Gfx_ColorRaw_t out = 0;
	for (u32 i = 0; i < WEUI_GFX_FILL_CHANNELS_NUM; i++) {
		u32 shift = i * WEUI_GFX_FILL_CHANNEL_BITS;
		s32 u	  = (s32)((under >> shift) & WEUI_GFX_FILL_CHANNEL_MASK);
		s32 o	  = (s32)((over >> shift) & WEUI_GFX_FILL_CHANNEL_MASK);
		s32 v	  = u + (((o - u) * a) >> WEUI_GFX_FILL_CHANNEL_BITS);
		out |= (WeUi_Gfx_ColorRaw_t)v << shift;
	}
	return out;
}

static WeUi_Gfx_Color_t WeUi_Gfx_Fill_GradientAt(const WeUi_Gfx_Paint_t* pcPaint, float t) {
	const WeUi_Gfx_PaintStop_t* pcStops = pcPaint->pcStops;
	u32 last							= pcPaint->StopsNum - 1u;

	if (t <= pcStops[0].Offset)
		return pcStops[0].Color;
	if (t >= pcStops[last].Offset)
		return pcStops[last].Color;

	u32 k = 0;
	while (t > pcStops[k + 1].Offset)
		k++;
	float span	= pcStops[k + 1].Offset - pcStops[k].Offset;
	float share = span > 0.0f ? (t - pcStops[k].Offset) / span : 1.0f;
	WeUi_Gfx_ColorRaw_t mix =
		WeUi_Gfx_Fill_Blend(WeUi_Gfx_Transform_Color2Bytes(pcStops[k].Color),
							WeUi_Gfx_Transform_Color2Bytes(pcStops[k + 1].Color),
							(u32)(share * WEUI_GFX_RASTER_COVER_FULL + WEUI_GFX_FILL_PIXEL_MID));
	return WeUi_Gfx_Transform_Bytes2Color(mix);
}

/**
 * @brief The paint's colours as the driver stores them: each drawn once on a pixel of the
 *        fill and read back, the pixel put back as it was. The driver alone knows how it
 *        stores a colour.
 */
static WeUi_Gfx_Color_t WeUi_Gfx_Fill_LutColor(const WeUi_Gfx_Paint_t* pcPaint, u32 i) {
	if (pcPaint->Type == WEUI_GFX_PAINT_SOLID)
		return pcPaint->Color;
	return WeUi_Gfx_Fill_GradientAt(pcPaint, (float)i / WEUI_GFX_FILL_LUT_LAST);
}

static u32 WeUi_Gfx_Fill_LutSize(const WeUi_Gfx_Paint_t* pcPaint) {
	return pcPaint->Type == WEUI_GFX_PAINT_SOLID ? 1u : WEUI_GFX_FILL_LUT_SIZE;
}

/** @brief Where the paint's colours start in the table */
static u32 WeUi_Gfx_Fill_LutBase(const WeUi_Gfx_Paint_t* pcPaint) {
	return pcPaint->Type == WEUI_GFX_PAINT_SOLID ? WEUI_GFX_FILL_LUT_SOLID : 0u;
}

/* 0x00BBGGRR whatever the library's scheme: a gray one keeps its level in R alone */
static WeUi_Gfx_ColorRaw_t WeUi_Gfx_Fill_ToRgb(WeUi_Gfx_Color_t color) {
#if defined(WEUI_LIB_COLOR_SCHEME__GRAY)
	return WeUi_Gfx_Transform_FillBytesFromChannel(color.R);
#else
	return WeUi_Gfx_Transform_Color2Bytes(color);
#endif
}

/**
 * @brief The paint's colours as the driver stores them: each drawn once on a pixel of the
 *        fill and read back, the pixel put back as it was. The driver alone knows how it
 *        stores a colour.
 */
static void WeUi_Gfx_Fill_Probe(WeUi_Gfx_FillCtx_t* pCtx, const WeUi_Gfx_Point_t* pcPt) {
	const WeUi_Gfx_Paint_t* pcPaint = pCtx->pcPaint;
	WeUi_Gfx_ColorRaw_t was			= WeUi_Gfx_Adapter_GetPixelRaw(pcPt);
	u32 base						= WeUi_Gfx_Fill_LutBase(pcPaint);

	if (base == 0)
		LutKeys[0][0].Valid = false;
	for (u32 i = 0; i < WeUi_Gfx_Fill_LutSize(pcPaint); i++) {
		WeUi_Gfx_ColorPoint_t cpt = {.Pt = *pcPt, .Color = WeUi_Gfx_Fill_LutColor(pcPaint, i)};
		WeUi_Gfx_Adapter_SetPixel(&cpt);
		Lut[0][base + i] = WeUi_Gfx_Adapter_GetPixelRaw(pcPt);
	}
	WeUi_Gfx_Adapter_SetPixelRaw(pcPt, was);
	pCtx->Probed = true;
}

static u16 WeUi_Gfx_Fill_To565(WeUi_Gfx_ColorRaw_t rgb, WEUI_GFX_TARGET_t format) {
	u32 r  = rgb & WEUI_GFX_FILL_CHANNEL_MASK;
	u32 g  = (rgb >> WEUI_GFX_FILL_G_AT) & WEUI_GFX_FILL_CHANNEL_MASK;
	u32 b  = (rgb >> WEUI_GFX_FILL_B_AT) & WEUI_GFX_FILL_CHANNEL_MASK;
	u16 px = (u16)(((r & WEUI_GFX_FILL_565_R_MASK) << WEUI_GFX_FILL_565_R_SHIFT) |
				   ((g & WEUI_GFX_FILL_565_G_MASK) << WEUI_GFX_FILL_565_G_SHIFT) |
				   (b >> WEUI_GFX_FILL_565_B_SHIFT));
	return format == WEUI_GFX_TARGET_RGB565_BE ? __builtin_bswap16(px) : px;
}

static WeUi_Gfx_ColorRaw_t WeUi_Gfx_Fill_From565(u16 px, WEUI_GFX_TARGET_t format) {
	if (format == WEUI_GFX_TARGET_RGB565_BE)
		px = __builtin_bswap16(px);
	u32 r = (px >> WEUI_GFX_FILL_565_R_SHIFT) & WEUI_GFX_FILL_565_R_MASK;
	u32 g = (px >> WEUI_GFX_FILL_565_G_SHIFT) & WEUI_GFX_FILL_565_G_MASK;
	u32 b = (u32)(px << WEUI_GFX_FILL_565_B_SHIFT) & WEUI_GFX_FILL_565_R_MASK;
	/* The top bits again in the bits a 565 channel lacks: full is 255, not 248 */
	r |= r >> WEUI_GFX_FILL_5_REPEAT;
	g |= g >> WEUI_GFX_FILL_6_REPEAT;
	b |= b >> WEUI_GFX_FILL_5_REPEAT;
	return r | (g << WEUI_GFX_FILL_G_AT) | (b << WEUI_GFX_FILL_B_AT);
}

/* A buffer takes the colours as they are: brightness is the panel's own */
/** @brief The gradient's key into pKey. @return false for one of too many stops to keep */
static bool WeUi_Gfx_Fill_KeyOf(const WeUi_Gfx_Paint_t* pcPaint, WEUI_GFX_TARGET_t format,
								WeUi_Gfx_FillLutKey_t* pKey) {
	if (pcPaint->StopsNum > WEUI_GFX_FILL_KEY_STOPS)
		return false;
	*pKey = (WeUi_Gfx_FillLutKey_t){.Valid = true, .Format = format, .StopsNum = pcPaint->StopsNum};
	for (u32 i = 0; i < pcPaint->StopsNum; i++) {
		pKey->Offsets[i] = pcPaint->pcStops[i].Offset;
		pKey->Colors[i]	 = WeUi_Gfx_Transform_Color2Bytes(pcPaint->pcStops[i].Color);
	}
	return true;
}

static bool WeUi_Gfx_Fill_KeyIs(const WeUi_Gfx_FillLutKey_t* pcKept,
								const WeUi_Gfx_FillLutKey_t* pcKey) {
	if (!pcKept->Valid || pcKept->Format != pcKey->Format || pcKept->StopsNum != pcKey->StopsNum)
		return false;
	for (u32 i = 0; i < pcKey->StopsNum; i++) {
		if (pcKept->Offsets[i] != pcKey->Offsets[i] || pcKept->Colors[i] != pcKey->Colors[i])
			return false;
	}
	return true;
}

static void WeUi_Gfx_Fill_LutForBuffer(WeUi_Gfx_FillCtx_t* pCtx) {
	const WeUi_Gfx_Paint_t* pcPaint = pCtx->pcPaint;
	WEUI_GFX_TARGET_t format		= pCtx->pcTarget->Format;
	const u8 id						= pCtx->RasterId;
	u32 base						= WeUi_Gfx_Fill_LutBase(pcPaint);
	pCtx->Probed					= true;

	WeUi_Gfx_FillLutKey_t key = {0};
	bool keep				  = base == 0 && WeUi_Gfx_Fill_KeyOf(pcPaint, format, &key);
	for (u32 k = 0; keep && k < WEUI_GFX_FILL_LUTS; k++) {
		if (WeUi_Gfx_Fill_KeyIs(&LutKeys[id][k], &key)) {
			pCtx->LutAt = k * WEUI_GFX_FILL_LUT_SIZE;
			return;
		}
	}

	if (base == 0) {
		u32 table		   = LutNext[id]++ % WEUI_GFX_FILL_LUTS;
		base			   = table * WEUI_GFX_FILL_LUT_SIZE;
		pCtx->LutAt		   = base;
		LutKeys[id][table] = key;
	}
	for (u32 i = 0; i < WeUi_Gfx_Fill_LutSize(pcPaint); i++) {
		Lut[id][base + i]	 = WeUi_Gfx_Fill_ToRgb(WeUi_Gfx_Fill_LutColor(pcPaint, i));
		Lut565[id][base + i] = WeUi_Gfx_Fill_To565(Lut[id][base + i], format);
	}
}

static u16* WeUi_Gfx_Fill_At(const WeUi_Gfx_Target_t* pcTarget, WeUi_Gfx_PtPos_t x,
							 WeUi_Gfx_PtPos_t y) {
	return &pcTarget->pBuf[(u32)(y - pcTarget->Rect.Y0) * pcTarget->Stride +
						   (u32)(x - pcTarget->Rect.X0)];
}

/** @brief n pixels of one value, two to a 32-bit write once aligned */
static void WeUi_Gfx_Fill_Run(u16* pPx, u32 n, u16 value) {
	/* Black and white have both bytes alike: the library's memset is quicker than any loop */
	if ((value >> WEUI_GFX_FILL_CHANNEL_BITS) == (value & WEUI_GFX_FILL_CHANNEL_MASK)) {
		memset(pPx, value & WEUI_GFX_FILL_CHANNEL_MASK, n * sizeof(*pPx));
		return;
	}
	if (n > 0 && ((uintptr_t)pPx & WEUI_GFX_FILL_PAIR_ALIGN) != 0) {
		*pPx++ = value;
		n--;
	}
	u32* pPair = (u32*)pPx;
	u32 pair   = value | ((u32)value << WEUI_GFX_FILL_PAIR_BITS);
	for (u32 i = 0; i < n / 2u; i++)
		pPair[i] = pair;
	if (n & 1u)
		pPx[n - 1u] = value;
}

static u32 WeUi_Gfx_Fill_OffsetIdx(float t) {
	t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	return (u32)(t * WEUI_GFX_FILL_LUT_LAST + WEUI_GFX_FILL_PIXEL_MID);
}

/** @brief A gradient's offset at the pixel, not yet clamped */
static float WeUi_Gfx_Fill_OffsetAt(const WeUi_Gfx_FillCtx_t* pcCtx, WeUi_Gfx_PtPos_t x,
									WeUi_Gfx_PtPos_t y) {
	WeUi_Gfx_PointF_t onPanel = {(float)x + WEUI_GFX_FILL_PIXEL_MID,
								 (float)y + WEUI_GFX_FILL_PIXEL_MID};
	WeUi_Gfx_PointF_t p		  = WeUi_Gfx_Affine_Apply(&pcCtx->Inv, onPanel);
	if (pcCtx->pcPaint->Type == WEUI_GFX_PAINT_LINEAR)
		return pcCtx->Gx * p.X + pcCtx->Gy * p.Y + pcCtx->G0;

	WeUi_Gfx_PointF_t c = pcCtx->pcPaint->P0;
	return sqrtf((p.X - c.X) * (p.X - c.X) + (p.Y - c.Y) * (p.Y - c.Y)) * pcCtx->InvRadius;
}

static u32 WeUi_Gfx_Fill_LutIdx(const WeUi_Gfx_FillCtx_t* pcCtx, WeUi_Gfx_PtPos_t x,
								WeUi_Gfx_PtPos_t y) {
	if (pcCtx->pcPaint->Type == WEUI_GFX_PAINT_SOLID)
		return WEUI_GFX_FILL_LUT_SOLID;
	return pcCtx->LutAt + WeUi_Gfx_Fill_OffsetIdx(WeUi_Gfx_Fill_OffsetAt(pcCtx, x, y));
}

static void WeUi_Gfx_Fill_Put(WeUi_Gfx_FillCtx_t* pCtx, WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y,
							  u8 cover) {
	u32 share = ((u32)cover * pCtx->Opacity) >> WEUI_GFX_FILL_CHANNEL_BITS;
	for (u32 i = 0; i < pCtx->pcPaint->MasksNum && share > 0; i++) {
		const WeUi_Gfx_MaskUse_t* pcUse = &pCtx->pcPaint->pcMasks[i];
		u32 m							= WeUi_Gfx_Mask_At(pcUse->pcMask, x, y);
		if (pcUse->Mode == WEUI_GFX_MASK_OUT)
			m = WEUI_GFX_RASTER_COVER_FULL - m;
		share = (share * WeUi_Gfx_Fill_ToMul(m)) >> WEUI_GFX_FILL_CHANNEL_BITS;
	}
	if (share == 0)
		return;

	const WeUi_Gfx_Target_t* pcTarget = pCtx->pcTarget;
	if (pcTarget) {
		WeUi_Gfx_ColorRaw_t over = Lut[pCtx->RasterId][WeUi_Gfx_Fill_LutIdx(pCtx, x, y)];
		u16* pPx				 = WeUi_Gfx_Fill_At(pcTarget, x, y);
		if (share < WEUI_GFX_RASTER_COVER_FULL)
			over = WeUi_Gfx_Fill_Blend(WeUi_Gfx_Fill_From565(*pPx, pcTarget->Format), over, share);
		*pPx = WeUi_Gfx_Fill_To565(over, pcTarget->Format);
		return;
	}

	WeUi_Gfx_Point_t pt = {.X = x, .Y = y};
	if (!pCtx->Probed)
		WeUi_Gfx_Fill_Probe(pCtx, &pt);

	WeUi_Gfx_ColorRaw_t stored = Lut[0][WeUi_Gfx_Fill_LutIdx(pCtx, x, y)];
	if (share >= WEUI_GFX_RASTER_COVER_FULL) {
		WeUi_Gfx_Adapter_SetPixelRaw(&pt, stored);
		return;
	}
	WeUi_Gfx_ColorRaw_t under = WeUi_Gfx_Adapter_GetPixelRaw(&pt);
	WeUi_Gfx_Adapter_SetPixelRaw(&pt, WeUi_Gfx_Fill_Blend(under, stored, share));
}

/** @brief The masks' rows at y. @return false when a mask kept inside has no row there */
static bool WeUi_Gfx_Fill_MaskRows(const WeUi_Gfx_Paint_t* pcPaint, WeUi_Gfx_PtPos_t y,
								   WeUi_Gfx_FillMaskRow_t* pRows, u32* pNum) {
	*pNum = 0;
	for (u32 i = 0; i < pcPaint->MasksNum; i++) {
		const WeUi_Gfx_MaskUse_t* pcUse = &pcPaint->pcMasks[i];
		const WeUi_Gfx_ClipRect_t* pcR	= &pcUse->pcMask->Rect;
		if (y < pcR->Y0 || y >= pcR->Y1) {
			if (pcUse->Mode == WEUI_GFX_MASK_IN)
				return false;
			continue;
		}
		pRows[(*pNum)++] = (WeUi_Gfx_FillMaskRow_t){
			.pcRow = &pcUse->pcMask->pBuf[(u32)(y - pcR->Y0) * (u32)(pcR->X1 - pcR->X0)],
			.X0	   = pcR->X0,
			.X1	   = pcR->X1,
			.Out   = pcUse->Mode == WEUI_GFX_MASK_OUT,
		};
	}
	return true;
}

/** @brief One channel from under towards over by a (0..256) - as WeUi_Gfx_Fill_Blend() does */
static inline u32 WeUi_Gfx_Fill_Mix(u32 under, u32 over, s32 a) {
	return (u32)((s32)under + ((((s32)over - (s32)under) * a) >> WEUI_GFX_FILL_CHANNEL_BITS));
}

static WeUi_Gfx_FillRgb_t WeUi_Gfx_Fill_Channels(WeUi_Gfx_ColorRaw_t rgb) {
	return (WeUi_Gfx_FillRgb_t){
		.R = rgb & WEUI_GFX_FILL_CHANNEL_MASK,
		.G = (rgb >> WEUI_GFX_FILL_G_AT) & WEUI_GFX_FILL_CHANNEL_MASK,
		.B = (rgb >> WEUI_GFX_FILL_B_AT) & WEUI_GFX_FILL_CHANNEL_MASK,
	};
}

/**
 * @brief WeUi_Gfx_Fill_Blend() of a buffer's pixel, to the same bits, a channel at a time: the
 *        pixel's channels widened as WeUi_Gfx_Fill_From565() does, mixed and cut back to 565.
 *        No colour is packed in between: packing and taking apart again cost more than the mix
 */
static inline u16 WeUi_Gfx_Fill_Blend565(u16 px, WeUi_Gfx_FillRgb_t over, u32 share, bool be) {
	u32 v	= be ? __builtin_bswap16(px) : px;
	u32 r	= (v >> WEUI_GFX_FILL_565_R_SHIFT) & WEUI_GFX_FILL_565_R_MASK;
	u32 g	= (v >> WEUI_GFX_FILL_565_G_SHIFT) & WEUI_GFX_FILL_565_G_MASK;
	u32 b	= (v << WEUI_GFX_FILL_565_B_SHIFT) & WEUI_GFX_FILL_565_R_MASK;
	s32 a	= (s32)WeUi_Gfx_Fill_ToMul(share);
	r		= WeUi_Gfx_Fill_Mix(r | (r >> WEUI_GFX_FILL_5_REPEAT), over.R, a);
	g		= WeUi_Gfx_Fill_Mix(g | (g >> WEUI_GFX_FILL_6_REPEAT), over.G, a);
	b		= WeUi_Gfx_Fill_Mix(b | (b >> WEUI_GFX_FILL_5_REPEAT), over.B, a);
	u32 out = ((r & WEUI_GFX_FILL_565_R_MASK) << WEUI_GFX_FILL_565_R_SHIFT) |
			  ((g & WEUI_GFX_FILL_565_G_MASK) << WEUI_GFX_FILL_565_G_SHIFT) |
			  (b >> WEUI_GFX_FILL_565_B_SHIFT);
	return be ? __builtin_bswap16((u16)out) : (u16)out;
}

/** @brief What every span of the row shares: the masks' rows, where the masks kept inside let
 *         the paint show, the row's pixels and a colour that does not change along it */
static void WeUi_Gfx_Fill_Row(WeUi_Gfx_FillCtx_t* pCtx, WeUi_Gfx_PtPos_t y) {
	const WeUi_Gfx_Target_t* pcTarget = pCtx->pcTarget;
	pCtx->RowY						  = y;
	pCtx->RowShown = WeUi_Gfx_Fill_MaskRows(pCtx->pcPaint, y, pCtx->RowMasks, &pCtx->RowMasksNum);
	pCtx->RowX0	   = pcTarget->Rect.X0;
	pCtx->RowX1	   = pcTarget->Rect.X1;
	for (u32 k = 0; k < pCtx->RowMasksNum; k++) {
		if (pCtx->RowMasks[k].Out)
			continue;
		pCtx->RowX0 = GET_MAX(pCtx->RowX0, pCtx->RowMasks[k].X0);
		pCtx->RowX1 = GET_MIN(pCtx->RowX1, pCtx->RowMasks[k].X1);
	}
	pCtx->pRowPx = WeUi_Gfx_Fill_At(pcTarget, pcTarget->Rect.X0, y);
	if (!pCtx->Fixed)
		return;
	u32 at		   = WeUi_Gfx_Fill_LutIdx(pCtx, pcTarget->Rect.X0, y);
	pCtx->RowWhole = Lut565[pCtx->RasterId][at];
	pCtx->RowOver  = WeUi_Gfx_Fill_Channels(Lut[pCtx->RasterId][at]);
}

/* A span's pixels' shares of the paint, one fill at a time */
static u8 WEUI_DATA_PLACEMENT SpanShare[WEUI_GFX_RASTER_SETS][WEUI_GFX_RASTER_WIDTH_MAX];
/* A soft spot's coverage along a row, to go out as a span */
static u8 WEUI_DATA_PLACEMENT SoftCov[WEUI_GFX_RASTER_SETS][WEUI_GFX_RASTER_WIDTH_MAX];

/** @brief The span's shares in SpanShare, [from, to), times each mask's row */
static void WeUi_Gfx_Fill_MaskShares(WeUi_Gfx_FillCtx_t* pCtx, const WeUi_Gfx_FillMaskRow_t* pcRows,
									 u32 rowsNum, WeUi_Gfx_PtPos_t x, s32 from, s32 to) {
	u8* pShare = SpanShare[pCtx->RasterId];
	for (u32 k = 0; k < rowsNum; k++) {
		const WeUi_Gfx_FillMaskRow_t* pcRow = &pcRows[k];
		s32 at								= x - pcRow->X0; /* the mask's pixel under pixel 0 */
		s32 a								= GET_MAX(from, pcRow->X0 - x);
		s32 b								= GET_MIN(to, pcRow->X1 - x);
		for (s32 i = a; i < b; i++) {
			u32 m	  = pcRow->pcRow[at + i];
			m		  = pcRow->Out ? WEUI_GFX_RASTER_COVER_FULL - m : m;
			pShare[i] = (u8)((pShare[i] * WeUi_Gfx_Fill_ToMul(m)) >> WEUI_GFX_FILL_CHANNEL_BITS);
		}
	}
}

static u32 WeUi_Gfx_Fill_LinearIdx(const WeUi_Gfx_FillCtx_t* ctx, float t0, s32 i) {
	return WeUi_Gfx_Fill_OffsetIdx(t0 + (float)i * ctx->DtDx);
}

#define WEUI_GFX_FILL_INDEX_BITS 20
#define WEUI_GFX_FILL_INDEX_ONE	 (1u << WEUI_GFX_FILL_INDEX_BITS)

static bool WeUi_Gfx_Fill_LinearBuffer(const WeUi_Gfx_FillCtx_t* ctx, u16* pixels, const u8* shares,
									   float t0, s32 from, s32 to, bool be) {
	const float start = (t0 + (float)from * ctx->DtDx) * WEUI_GFX_FILL_LUT_LAST + .5f;
	const float end	  = (t0 + (float)(to - 1) * ctx->DtDx) * WEUI_GFX_FILL_LUT_LAST + .5f;
	const float step  = ctx->DtDx * WEUI_GFX_FILL_LUT_LAST;
	const float limit = (float)INT32_MAX / WEUI_GFX_FILL_INDEX_ONE - 2;
	/* Include float evaluation error and accumulated Q20 truncation in the uncertain band. */
	const float error = ((fabsf(t0) + fabsf(ctx->DtDx) * (float)to) * WEUI_GFX_FILL_LUT_LAST + 1) *
							(8 * FLT_EPSILON * WEUI_GFX_FILL_INDEX_ONE) +
						(float)(to - from + 2);
	if (!(fabsf(start) < limit && fabsf(end) < limit && fabsf(step) < limit &&
		  error < (float)WEUI_GFX_FILL_INDEX_ONE / 2.0f))
		return false;
	s32 cursor		  = (s32)(start * WEUI_GFX_FILL_INDEX_ONE);
	const s32 advance = (s32)(step * WEUI_GFX_FILL_INDEX_ONE);
	const u32 guard	  = (u32)error + 1;
	for (s32 i = from; i < to; i++) {
		const u32 share = shares[i];
		if (share > 0) {
			const u32 fraction = (u32)cursor & (WEUI_GFX_FILL_INDEX_ONE - 1);
			u32 idx;
			/* Near a bin boundary, retain the scalar path's exact float rounding. */
			if (fraction <= guard || fraction >= WEUI_GFX_FILL_INDEX_ONE - guard)
				idx = WeUi_Gfx_Fill_LinearIdx(ctx, t0, i);
			else
				idx = cursor <= 0 ? 0
					  : cursor >= (s32)(WEUI_GFX_FILL_LUT_LAST * WEUI_GFX_FILL_INDEX_ONE)
						  ? WEUI_GFX_FILL_LUT_LAST
						  : (u32)cursor >> WEUI_GFX_FILL_INDEX_BITS;
			const u32 at = ctx->LutAt + idx;
			pixels[i] = share >= WEUI_GFX_RASTER_COVER_FULL
							? Lut565[ctx->RasterId][at]
							: WeUi_Gfx_Fill_Blend565(pixels[i],
													 WeUi_Gfx_Fill_Channels(Lut[ctx->RasterId][at]),
													 share, be);
		}
		if (i + 1 < to)
			cursor += advance;
	}
	return true;
}

/**
 * @brief A span into a buffer, in two loops. The first works out each pixel's share of the
 *        paint, a mask at a time, only where every mask kept inside reaches - an opaque paint
 *        without masks skips it: its coverage is the share. The second writes the pixels: a
 *        pixel the paint covers whole from the table as it is, only an edge blended. The masks'
 *        rows and a colour that does not change along the row are found with the row's first
 *        span; a linear gradient's offset that does is stepped along the span.
 */
static void WeUi_Gfx_Fill_SpanBuffer(WeUi_Gfx_FillCtx_t* pCtx, WeUi_Gfx_PtPos_t y,
									 WeUi_Gfx_PtPos_t x, u16 len, const u8* pcCov, u8 cov) {
	if (y != pCtx->RowY)
		WeUi_Gfx_Fill_Row(pCtx, y);
	if (!pCtx->RowShown)
		return;

	s32 from = GET_MAX(0, pCtx->RowX0 - x);
	s32 to	 = GET_MIN((s32)len, pCtx->RowX1 - x);
	if (from >= to)
		return;

	u32 rowsNum		  = pCtx->RowMasksNum;
	const u8* pcShare = pcCov;
	if (rowsNum > 0 || pcCov == NULL ||
		pCtx->Opacity != WeUi_Gfx_Fill_ToMul(WEUI_GFX_PAINT_OPAQUE)) {
		for (s32 i = from; i < to; i++)
			SpanShare[pCtx->RasterId][i] =
				(u8)(((u32)(pcCov ? pcCov[i] : cov) * pCtx->Opacity) >> WEUI_GFX_FILL_CHANNEL_BITS);
		WeUi_Gfx_Fill_MaskShares(pCtx, pCtx->RowMasks, rowsNum, x, from, to);
		pcShare = SpanShare[pCtx->RasterId];
	}

	u16* pPx = pCtx->pRowPx + (x - pCtx->pcTarget->Rect.X0);
	bool be	 = pCtx->pcTarget->Format == WEUI_GFX_TARGET_RGB565_BE;

	if (pCtx->Fixed) {
		u16 whole				= pCtx->RowWhole;
		WeUi_Gfx_FillRgb_t over = pCtx->RowOver;
		for (s32 i = from; i < to; i++) {
			u32 share = pcShare[i];
			if (share == 0)
				continue;
			pPx[i] = share >= WEUI_GFX_RASTER_COVER_FULL
						 ? whole
						 : WeUi_Gfx_Fill_Blend565(pPx[i], over, share, be);
		}
		return;
	}

	bool linear = pCtx->pcPaint->Type == WEUI_GFX_PAINT_LINEAR;
	float t0	= linear ? WeUi_Gfx_Fill_OffsetAt(pCtx, x, y) : 0.0f;
#ifndef WEUI_GFX_FILL_TEST_REFERENCE
	if (linear) {
		const u32 first = WeUi_Gfx_Fill_LinearIdx(pCtx, t0, from);
		const u32 last	= WeUi_Gfx_Fill_LinearIdx(pCtx, t0, to - 1);
		if (first == last) {
			const u32 at				  = pCtx->LutAt + first;
			const u16 whole				  = Lut565[pCtx->RasterId][at];
			const WeUi_Gfx_FillRgb_t over = WeUi_Gfx_Fill_Channels(Lut[pCtx->RasterId][at]);
			for (s32 i = from; i < to; i++) {
				const u32 share = pcShare[i];
				if (share > 0) {
					pPx[i] = share >= WEUI_GFX_RASTER_COVER_FULL
								 ? whole
								 : WeUi_Gfx_Fill_Blend565(pPx[i], over, share, be);
				}
			}
			return;
		}
		if (WeUi_Gfx_Fill_LinearBuffer(pCtx, pPx, pcShare, t0, from, to, be))
			return;
	}
#endif
	for (s32 i = from; i < to; i++) {
		u32 share = pcShare[i];
		if (share == 0)
			continue;

		u32 at = linear ? pCtx->LutAt + WeUi_Gfx_Fill_OffsetIdx(t0 + (float)i * pCtx->DtDx)
						: WeUi_Gfx_Fill_LutIdx(pCtx, (WeUi_Gfx_PtPos_t)(x + i), y);
		pPx[i] = share >= WEUI_GFX_RASTER_COVER_FULL
					 ? Lut565[pCtx->RasterId][at]
					 : WeUi_Gfx_Fill_Blend565(
						   pPx[i], WeUi_Gfx_Fill_Channels(Lut[pCtx->RasterId][at]), share, be);
	}
}

static void WeUi_Gfx_Fill_Span(void* pCtx, WeUi_Gfx_PtPos_t y, WeUi_Gfx_PtPos_t x, u16 len,
							   const u8* pcCov, u8 cov) {
	const WeUi_Gfx_FillCtx_t* pcCtx = pCtx;
	if (pcCtx->Plain && pcCov == NULL && cov == WEUI_GFX_RASTER_COVER_FULL) {
		WeUi_Gfx_Fill_Run(WeUi_Gfx_Fill_At(pcCtx->pcTarget, x, y), len,
						  Lut565[pcCtx->RasterId][WEUI_GFX_FILL_LUT_SOLID]);
		return;
	}
	if (pcCtx->pcTarget && pcCtx->pcPaint->MasksNum <= WEUI_GFX_FILL_SPAN_MASKS) {
		WeUi_Gfx_Fill_SpanBuffer(pCtx, y, x, len, pcCov, cov);
		return;
	}
	for (u16 i = 0; i < len; i++)
		WeUi_Gfx_Fill_Put(pCtx, (WeUi_Gfx_PtPos_t)(x + i), y, pcCov ? pcCov[i] : cov);
}

/* The driver checks no coordinates: whatever clip is asked for stays on the panel */
static WeUi_Gfx_ClipRect_t WeUi_Gfx_Fill_OnPanel(const WeUi_Gfx_ClipRect_t* pcClip) {
	WeUi_Gfx_ClipRect_t panel = WeUi_Gfx_Fill_PanelClip();
	if (pcClip == NULL)
		return panel;

	return (WeUi_Gfx_ClipRect_t){
		.X0 = GET_MAX(pcClip->X0, panel.X0),
		.Y0 = GET_MAX(pcClip->Y0, panel.Y0),
		.X1 = GET_MIN(pcClip->X1, panel.X1),
		.Y1 = GET_MIN(pcClip->Y1, panel.Y1),
	};
}

static RET_STATE_t WeUi_Gfx_Fill_InvertXf(const WeUi_Gfx_Affine_t* pcXf, WeUi_Gfx_Affine_t* pInv) {
	if (pcXf == NULL) {
		*pInv = WeUi_Gfx_Affine_Identity();
		return RET_STATE_SUCCESS;
	}
	return WeUi_Gfx_Affine_Invert(pcXf, pInv);
}

static bool WeUi_Gfx_Fill_StopsInOrder(const WeUi_Gfx_Paint_t* pcPaint) {
	if (pcPaint->pcStops == NULL || pcPaint->StopsNum == 0)
		return false;

	for (u32 i = 1; i < pcPaint->StopsNum; i++) {
		if (pcPaint->pcStops[i].Offset < pcPaint->pcStops[i - 1].Offset)
			return false;
	}
	return true;
}

static bool WeUi_Gfx_Fill_MasksThere(const WeUi_Gfx_Paint_t* pcPaint) {
	if (pcPaint->MasksNum > 0 && pcPaint->pcMasks == NULL)
		return false;

	for (u32 i = 0; i < pcPaint->MasksNum; i++) {
		const WeUi_Gfx_Mask_t* pcMask = pcPaint->pcMasks[i].pcMask;
		if (pcMask == NULL || pcMask->pBuf == NULL)
			return false;
	}
	return true;
}

/** @brief The clip on the panel, and inside every mask the paint keeps in: nothing shows past
 *         them */
static WeUi_Gfx_ClipRect_t WeUi_Gfx_Fill_Clip(const WeUi_Gfx_ClipRect_t* pcClip,
											  const WeUi_Gfx_Paint_t* pcPaint,
											  const WeUi_Gfx_Target_t* pcTarget) {
	WeUi_Gfx_ClipRect_t clip = WeUi_Gfx_Fill_OnPanel(pcClip);
	if (pcTarget) {
		const WeUi_Gfx_ClipRect_t* pcR = &pcTarget->Rect;
		clip						   = (WeUi_Gfx_ClipRect_t){
			.X0 = pcClip ? GET_MAX(pcClip->X0, pcR->X0) : pcR->X0,
			.Y0 = pcClip ? GET_MAX(pcClip->Y0, pcR->Y0) : pcR->Y0,
			.X1 = pcClip ? GET_MIN(pcClip->X1, pcR->X1) : pcR->X1,
			.Y1 = pcClip ? GET_MIN(pcClip->Y1, pcR->Y1) : pcR->Y1,
		};
	}
	for (u32 i = 0; i < pcPaint->MasksNum; i++) {
		if (pcPaint->pcMasks[i].Mode != WEUI_GFX_MASK_IN)
			continue;
		const WeUi_Gfx_ClipRect_t* pcIn = &pcPaint->pcMasks[i].pcMask->Rect;
		clip.X0							= GET_MAX(clip.X0, pcIn->X0);
		clip.Y0							= GET_MAX(clip.Y0, pcIn->Y0);
		clip.X1							= GET_MIN(clip.X1, pcIn->X1);
		clip.Y1							= GET_MIN(clip.Y1, pcIn->Y1);
	}
	return clip;
}

/** @brief Check the paint and work out what every pixel of the fill shares */
static RET_STATE_t WeUi_Gfx_Fill_Check(const WeUi_Gfx_Paint_t* pcPaint, WeUi_Gfx_FillCtx_t* pCtx) {
	*pCtx = (WeUi_Gfx_FillCtx_t){
		.pcPaint  = pcPaint,
		.Opacity  = WeUi_Gfx_Fill_ToMul(pcPaint->Opacity),
		.Fixed	  = pcPaint->Type == WEUI_GFX_PAINT_SOLID,
		.RowY	  = WEUI_GFX_FILL_ROW_NONE,
		.RasterId = 0,
	};
	if (!WeUi_Gfx_Fill_MasksThere(pcPaint))
		return RET_STATE_ERR_PARAM;
	if (pcPaint->Type == WEUI_GFX_PAINT_SOLID)
		return RET_STATE_SUCCESS;

	if (!WeUi_Gfx_Fill_StopsInOrder(pcPaint))
		return RET_STATE_ERR_PARAM;
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Fill_InvertXf(pcPaint->pcXf, &pCtx->Inv));

	if (pcPaint->Type == WEUI_GFX_PAINT_RADIAL) {
		if (!(pcPaint->Radius > 0.0f))
			return RET_STATE_ERR_PARAM;
		pCtx->InvRadius = 1.0f / pcPaint->Radius;
		return RET_STATE_SUCCESS;
	}

	float dx   = pcPaint->P1.X - pcPaint->P0.X;
	float dy   = pcPaint->P1.Y - pcPaint->P0.Y;
	float len2 = dx * dx + dy * dy;
	if (!(len2 > 0.0f))
		return RET_STATE_ERR_PARAM;
	pCtx->Gx	= dx / len2;
	pCtx->Gy	= dy / len2;
	pCtx->G0	= -(pcPaint->P0.X * dx + pcPaint->P0.Y * dy) / len2;
	pCtx->DtDx	= pCtx->Gx * pCtx->Inv.A + pCtx->Gy * pCtx->Inv.B;
	pCtx->Fixed = pCtx->DtDx == 0.0f;
	return RET_STATE_SUCCESS;
}

/** @brief Check the paint for the target and work out what every pixel of the fill shares */
static RET_STATE_t WeUi_Gfx_Fill_Prepare(const WeUi_Gfx_Paint_t* pcPaint,
										 const WeUi_Gfx_Target_t* pcTarget,
										 WeUi_Gfx_FillCtx_t* pCtx) {
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Fill_Check(pcPaint, pCtx));
	if (pcTarget == NULL)
		return RET_STATE_SUCCESS;
	if (pcTarget->pBuf == NULL || pcTarget->Stride < (u32)(pcTarget->Rect.X1 - pcTarget->Rect.X0))
		return RET_STATE_ERR_PARAM;

	pCtx->pcTarget = pcTarget;
	pCtx->RasterId = pcTarget->RasterId;
	pCtx->Plain	   = pcPaint->Type == WEUI_GFX_PAINT_SOLID &&
					 pcPaint->Opacity == WEUI_GFX_PAINT_OPAQUE && pcPaint->MasksNum == 0;
	WeUi_Gfx_Fill_LutForBuffer(pCtx);
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Paint_Check(const WeUi_Gfx_Paint_t* pcPaint) {
	WeUi_Gfx_FillCtx_t ctx;
	return WeUi_Gfx_Fill_Check(pcPaint, &ctx);
}

RET_STATE_t WeUi_Gfx_Fill_PaintShapeTo(const WeUi_Gfx_Shape_t* pcShape,
									   const WeUi_Gfx_Paint_t* pcPaint,
									   const WeUi_Gfx_ClipRect_t* pcClip,
									   const WeUi_Gfx_Target_t* pcTarget) {
	WeUi_Gfx_FillCtx_t ctx;
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Fill_Prepare(pcPaint, pcTarget, &ctx));

	WeUi_Gfx_ClipRect_t clip = WeUi_Gfx_Fill_Clip(pcClip, pcPaint, pcTarget);
	return WeUi_Gfx_Raster_FillShapeId(pcShape, &clip, WeUi_Gfx_Fill_Span, &ctx, ctx.RasterId);
}

RET_STATE_t WeUi_Gfx_Fill_PaintShape(const WeUi_Gfx_Shape_t* pcShape,
									 const WeUi_Gfx_Paint_t* pcPaint,
									 const WeUi_Gfx_ClipRect_t* pcClip) {
	return WeUi_Gfx_Fill_PaintShapeTo(pcShape, pcPaint, pcClip, NULL);
}

RET_STATE_t WeUi_Gfx_Fill_Paint(const WeUi_Gfx_Path_t* pcPath, const WeUi_Gfx_Paint_t* pcPaint,
								const WeUi_Gfx_ClipRect_t* pcClip) {
	if (pcPath->Broken)
		return RET_STATE_ERR_PARAM;

	WeUi_Gfx_Shape_t shape = WeUi_Gfx_Path_Shape(pcPath);
	return WeUi_Gfx_Fill_PaintShape(&shape, pcPaint, pcClip);
}

RET_STATE_t WeUi_Gfx_Fill_Solid(const WeUi_Gfx_Path_t* pcPath, WeUi_Gfx_Color_t color,
								const WeUi_Gfx_ClipRect_t* pcClip) {
	WeUi_Gfx_Paint_t paint = WeUi_Gfx_Paint_Solid(color);
	return WeUi_Gfx_Fill_Paint(pcPath, &paint, pcClip);
}

/* floorf and ceilf are library calls on some targets */
static s32 WeUi_Gfx_Fill_Floor(float v) {
	s32 i = (s32)v;
	return i - (v < (float)i);
}

static s32 WeUi_Gfx_Fill_Ceil(float v) {
	s32 i = (s32)v;
	return i + (v > (float)i);
}

RET_STATE_t WeUi_Gfx_Fill_SoftBounds(const WeUi_Gfx_Soft_t* pcSoft, WeUi_Gfx_ClipRect_t* pBounds) {
	if (!(pcSoft->Rx > 0.0f) || !(pcSoft->Ry > 0.0f))
		return RET_STATE_ERR_PARAM;

	WeUi_Gfx_Affine_t xf = pcSoft->pcXf ? *pcSoft->pcXf : WeUi_Gfx_Affine_Identity();
	/* The transformed ellipse's reach from its centre along each panel axis */
	WeUi_Gfx_PointF_t c	 = WeUi_Gfx_Affine_Apply(&xf, pcSoft->Centre);
	float halfX =
		sqrtf(xf.A * xf.A * pcSoft->Rx * pcSoft->Rx + xf.C * xf.C * pcSoft->Ry * pcSoft->Ry);
	float halfY =
		sqrtf(xf.B * xf.B * pcSoft->Rx * pcSoft->Rx + xf.D * xf.D * pcSoft->Ry * pcSoft->Ry);

	*pBounds = (WeUi_Gfx_ClipRect_t){
		.X0 = (WeUi_Gfx_PtPos_t)WeUi_Gfx_Fill_Floor(c.X - halfX),
		.Y0 = (WeUi_Gfx_PtPos_t)WeUi_Gfx_Fill_Floor(c.Y - halfY),
		.X1 = (WeUi_Gfx_PtPos_t)WeUi_Gfx_Fill_Ceil(c.X + halfX),
		.Y1 = (WeUi_Gfx_PtPos_t)WeUi_Gfx_Fill_Ceil(c.Y + halfY),
	};
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Fill_SoftTo(const WeUi_Gfx_Soft_t* pcSoft, const WeUi_Gfx_ClipRect_t* pcClip,
								 const WeUi_Gfx_Target_t* pcTarget) {
	WeUi_Gfx_ClipRect_t bounds;
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Fill_SoftBounds(pcSoft, &bounds));

	WeUi_Gfx_Affine_t xf = pcSoft->pcXf ? *pcSoft->pcXf : WeUi_Gfx_Affine_Identity();
	WeUi_Gfx_Affine_t inv;
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Affine_Invert(&xf, &inv));

	WeUi_Gfx_Paint_t paint = WeUi_Gfx_Paint_Solid(pcSoft->Color);
	paint.Opacity		   = pcSoft->Opacity;
	paint.pcMasks		   = pcSoft->pcMasks;
	paint.MasksNum		   = pcSoft->MasksNum;
	WeUi_Gfx_FillCtx_t ctx;
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Fill_Prepare(&paint, pcTarget, &ctx));

	WeUi_Gfx_ClipRect_t clip = WeUi_Gfx_Fill_Clip(pcClip, &paint, pcTarget);
	s32 x0					 = GET_MAX(clip.X0, bounds.X0);
	s32 x1					 = GET_MIN(clip.X1, bounds.X1);
	s32 y0					 = GET_MAX(clip.Y0, bounds.Y0);
	s32 y1					 = GET_MIN(clip.Y1, bounds.Y1);

	float invRx = 1.0f / pcSoft->Rx;
	float invRy = 1.0f / pcSoft->Ry;
	/* A pixel to the right moves the spot's coordinates by the inverse's first column */
	float du	= inv.A * invRx;
	float dv	= inv.B * invRy;
	/* A row's coverage at a time, out as spans: the masks and the blend go the quick way */
	u8* pCov	= SoftCov[ctx.RasterId];
	for (s32 y = y0; y < y1; y++) {
		for (s32 xs = x0; xs < x1; xs += WEUI_GFX_RASTER_WIDTH_MAX) {
			s32 n					  = GET_MIN(x1 - xs, WEUI_GFX_RASTER_WIDTH_MAX);
			WeUi_Gfx_PointF_t onPanel = {(float)xs + WEUI_GFX_FILL_PIXEL_MID,
										 (float)y + WEUI_GFX_FILL_PIXEL_MID};
			WeUi_Gfx_PointF_t p		  = WeUi_Gfx_Affine_Apply(&inv, onPanel);
			float u0				  = (p.X - pcSoft->Centre.X) * invRx;
			float v0				  = (p.Y - pcSoft->Centre.Y) * invRy;
			for (s32 i = 0; i < n; i++) {
				float u = u0 + (float)i * du;
				float v = v0 + (float)i * dv;
				float q = 1.0f - u * u - v * v;
				pCov[i] = q > 0.0f ? (u8)(q * q * WEUI_GFX_RASTER_COVER_FULL) : 0u;
			}
			WeUi_Gfx_Fill_Span(&ctx, (WeUi_Gfx_PtPos_t)y, (WeUi_Gfx_PtPos_t)xs, (u16)n, pCov, 0);
		}
	}
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Fill_Soft(const WeUi_Gfx_Soft_t* pcSoft, const WeUi_Gfx_ClipRect_t* pcClip) {
	return WeUi_Gfx_Fill_SoftTo(pcSoft, pcClip, NULL);
}
