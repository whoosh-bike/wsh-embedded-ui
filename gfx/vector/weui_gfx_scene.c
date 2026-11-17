#include "weui_gfx_scene.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_transform.h"

/* The fingerprint is an FNV-1a hash: these are that algorithm's own constants, not settings */
#define WEUI_GFX_SCENE_FNV_BASIS   2166136261u
#define WEUI_GFX_SCENE_FNV_PRIME   16777619u
/* MurmurHash3's finaliser, its own constants too: FNV is near linear, and a sum of near linear
 * hashes lets symmetric moves cancel out - the two lids of an eye closing, say */
#define WEUI_GFX_SCENE_FMIX_SHIFT1 16
#define WEUI_GFX_SCENE_FMIX_MUL1   0x85EBCA6Bu
#define WEUI_GFX_SCENE_FMIX_SHIFT2 13
#define WEUI_GFX_SCENE_FMIX_MUL2   0xC2B2AE35u
/* Numbers are rounded before they are hashed, or float noise in their last bits would make an
 * item look changed every frame. Positions and radii to 1/16 px: a drift under 1/32 px is not
 * worth a redraw. Numbers around 1 - a transform's scale and turn, a gradient's stop offsets -
 * to 1/1024, or a size changing by 6% would go unnoticed */
#define WEUI_GFX_SCENE_QUANT_PX	   16.0f
#define WEUI_GFX_SCENE_QUANT_FINE  1024.0f
#define WEUI_GFX_SCENE_ROUND	   0.5f
#define WEUI_GFX_SCENE_SHIFT	   8
/* Changed rectangles kept apart while they are merged; past this they merge anyway */
#define WEUI_GFX_SCENE_DAMAGE_KEPT 8
#define WEUI_GFX_SCENE_NO_ITEM	   UINT8_MAX

/* The masks rendered for the band being drawn */
static u8 WEUI_DATA_PLACEMENT
	CacheBufs[WEUI_GFX_SCENE_MASKS_CACHED][WEUI_GFX_RASTER_WIDTH_MAX * WEUI_GFX_RASTER_BAND_ROWS];
static WeUi_Gfx_Mask_t CacheMasks[WEUI_GFX_SCENE_MASKS_CACHED];
static u8 CacheItem[WEUI_GFX_SCENE_MASKS_CACHED];
static u32 CacheNext;

// =================== Rectangles ===================

static bool WeUi_Gfx_Scene_IsEmpty(const WeUi_Gfx_ClipRect_t* pcR) {
	return pcR->X0 >= pcR->X1 || pcR->Y0 >= pcR->Y1;
}

static WeUi_Gfx_ClipRect_t WeUi_Gfx_Scene_Cut(WeUi_Gfx_ClipRect_t a,
											  const WeUi_Gfx_ClipRect_t* pcB) {
	a.X0 = GET_MAX(a.X0, pcB->X0);
	a.Y0 = GET_MAX(a.Y0, pcB->Y0);
	a.X1 = GET_MIN(a.X1, pcB->X1);
	a.Y1 = GET_MIN(a.Y1, pcB->Y1);
	return a;
}

static WeUi_Gfx_ClipRect_t WeUi_Gfx_Scene_Join(WeUi_Gfx_ClipRect_t a,
											   const WeUi_Gfx_ClipRect_t* pcB) {
	a.X0 = GET_MIN(a.X0, pcB->X0);
	a.Y0 = GET_MIN(a.Y0, pcB->Y0);
	a.X1 = GET_MAX(a.X1, pcB->X1);
	a.Y1 = GET_MAX(a.Y1, pcB->Y1);
	return a;
}

static bool WeUi_Gfx_Scene_Near(const WeUi_Gfx_ClipRect_t* pcA, const WeUi_Gfx_ClipRect_t* pcB,
								s32 gap) {
	return pcA->X0 <= pcB->X1 + gap && pcB->X0 <= pcA->X1 + gap && pcA->Y0 <= pcB->Y1 + gap &&
		   pcB->Y0 <= pcA->Y1 + gap;
}

/** @brief Add r to the list, merged with every one near it, and those with each other */
static void WeUi_Gfx_Scene_AddRect(WeUi_Gfx_ClipRect_t* pList, u32* pNum, WeUi_Gfx_ClipRect_t r,
								   s32 gap) {
	if (WeUi_Gfx_Scene_IsEmpty(&r))
		return;

	for (u32 i = 0; i < *pNum;) {
		if (!WeUi_Gfx_Scene_Near(&pList[i], &r, gap)) {
			i++;
			continue;
		}
		r		 = WeUi_Gfx_Scene_Join(r, &pList[i]);
		pList[i] = pList[--*pNum];
		i		 = 0; /* the grown one may now reach those passed over */
	}
	if (*pNum == WEUI_GFX_SCENE_DAMAGE_KEPT) {
		r		 = WeUi_Gfx_Scene_Join(r, &pList[0]);
		pList[0] = pList[--*pNum];
	}
	pList[(*pNum)++] = r;
}

static WeUi_Gfx_ClipRect_t WeUi_Gfx_Scene_Align(WeUi_Gfx_ClipRect_t r, s32 align) {
	s32 mask = ~(align - 1);
	r.X0	 = (WeUi_Gfx_PtPos_t)(r.X0 & mask);
	r.Y0	 = (WeUi_Gfx_PtPos_t)(r.Y0 & mask);
	r.X1	 = (WeUi_Gfx_PtPos_t)((r.X1 + align - 1) & mask);
	r.Y1	 = (WeUi_Gfx_PtPos_t)((r.Y1 + align - 1) & mask);
	return r;
}

// =================== Building ===================

void WeUi_Gfx_Scene_Init(WeUi_Gfx_Scene_t* pScene) {
	WeUi_Gfx_Scene_Begin(pScene);
	pScene->PrevNum		= 0;
	pScene->Shown		= false;
	pScene->DamageGap	= WEUI_GFX_SCENE_DAMAGE_GAP;
	pScene->DamageAlign = WEUI_GFX_SCENE_DAMAGE_ALIGN;
}

void WeUi_Gfx_Scene_Begin(WeUi_Gfx_Scene_t* pScene) {
	pScene->ItemsNum	= 0;
	pScene->PtsNum		= 0;
	pScene->ContoursNum = 0;
	pScene->StopsNum	= 0;
	pScene->Overflow	= false;
}

/** @brief The item's shape with its Bounds: a fill's or a mask's are the shape's own, worked out
 *         as it is added; an unseen fill's are empty, and it draws nothing */
static WeUi_Gfx_Shape_t WeUi_Gfx_Scene_Shape(const WeUi_Gfx_Scene_t* pcScene,
											 const WeUi_Gfx_SceneItem_t* pcItem) {
	return (WeUi_Gfx_Shape_t){
		.pcPts		 = pcScene->Pts,
		.pcContours	 = &pcScene->Contours[pcItem->Contour0],
		.ContoursNum = pcItem->ContoursNum,
		.pcBounds	 = &pcItem->Bounds,
	};
}

/** @brief A new item at the end, if there is room for it and the shape and stops it brings */
static RET_STATE_t WeUi_Gfx_Scene_New(WeUi_Gfx_Scene_t* pScene, WEUI_GFX_SCENE_ITEM_t kind,
									  const WeUi_Gfx_Path_t* pcPath, u32 stops,
									  WeUi_Gfx_SceneItem_t** ppItem, u8* pIdx) {
	u32 pts		 = pcPath ? pcPath->PtsNum : 0u;
	u32 contours = pcPath ? pcPath->ContoursNum : 0u;
	if (pScene->ItemsNum >= WEUI_GFX_SCENE_ITEMS_MAX ||
		pScene->PtsNum + pts > WEUI_GFX_SCENE_PTS_MAX ||
		pScene->ContoursNum + contours > WEUI_GFX_SCENE_CONTOURS_MAX ||
		pScene->StopsNum + stops > WEUI_GFX_SCENE_STOPS_MAX) {
		pScene->Overflow = true;
		return RET_STATE_ERR_OVERFLOW;
	}

	WeUi_Gfx_SceneItem_t* pItem = &pScene->Items[pScene->ItemsNum];
	*pItem						= (WeUi_Gfx_SceneItem_t){.Kind		  = kind,
														 .Contour0	  = pScene->ContoursNum,
														 .ContoursNum = (u16)contours,
														 .Stop0		  = pScene->StopsNum};

	/* A path's contours start in its own points: in the pool they start past the ones before */
	for (u32 c = 0; c < contours; c++) {
		WeUi_Gfx_PathContour_t contour = pcPath->Contours[c];
		contour.Start += pScene->PtsNum;
		pScene->Contours[pScene->ContoursNum++] = contour;
	}
	if (pts > 0)
		memcpy(&pScene->Pts[pScene->PtsNum], pcPath->Pts, pts * sizeof(WeUi_Gfx_PointF_t));
	pScene->PtsNum = (u16)(pScene->PtsNum + pts);

	if (pIdx)
		*pIdx = pScene->ItemsNum;
	pScene->ItemsNum++;
	*ppItem = pItem;
	return RET_STATE_SUCCESS;
}

static void WeUi_Gfx_Scene_KeepXf(WeUi_Gfx_SceneItem_t* pItem, const WeUi_Gfx_Affine_t* pcXf) {
	pItem->HasXf = pcXf != NULL;
	if (pcXf)
		pItem->Xf = *pcXf;
}

RET_STATE_t WeUi_Gfx_Scene_Fill(WeUi_Gfx_Scene_t* pScene, const WeUi_Gfx_Path_t* pcPath,
								const WeUi_Gfx_Paint_t* pcPaint, u8* pIdx) {
	if (pcPath->Broken || pcPaint->MasksNum > 0)
		return RET_STATE_ERR_PARAM;
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Paint_Check(pcPaint));

	u32 stops = pcPaint->Type == WEUI_GFX_PAINT_SOLID ? 0u : pcPaint->StopsNum;
	WeUi_Gfx_SceneItem_t* pItem;
	RETURN_IF_UNSUCCESS(
		WeUi_Gfx_Scene_New(pScene, WEUI_GFX_SCENE_FILL, pcPath, stops, &pItem, pIdx));

	if (stops > 0)
		memcpy(&pScene->Stops[pItem->Stop0], pcPaint->pcStops,
			   stops * sizeof(WeUi_Gfx_PaintStop_t));
	pScene->StopsNum = (u16)(pScene->StopsNum + stops);

	pItem->Paint		 = *pcPaint;
	pItem->Paint.pcStops = NULL;
	pItem->Paint.pcXf	 = NULL;
	WeUi_Gfx_Scene_KeepXf(pItem, pcPaint->pcXf);

	WeUi_Gfx_Shape_t shape = WeUi_Gfx_Scene_Shape(pScene, pItem);
	WeUi_Gfx_Raster_ShapeBounds(&shape, &pItem->Bounds);
	if (pcPaint->Opacity == 0)
		pItem->Bounds = (WeUi_Gfx_ClipRect_t){0};
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Scene_Soft(WeUi_Gfx_Scene_t* pScene, const WeUi_Gfx_Soft_t* pcSoft, u8* pIdx) {
	WeUi_Gfx_ClipRect_t bounds;
	if (pcSoft->MasksNum > 0)
		return RET_STATE_ERR_PARAM;
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Fill_SoftBounds(pcSoft, &bounds));
	if (pcSoft->pcXf) {
		WeUi_Gfx_Affine_t inv;
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Affine_Invert(pcSoft->pcXf, &inv));
	}

	WeUi_Gfx_SceneItem_t* pItem;
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Scene_New(pScene, WEUI_GFX_SCENE_SOFT, NULL, 0, &pItem, pIdx));
	pItem->Soft		 = *pcSoft;
	pItem->Soft.pcXf = NULL;
	WeUi_Gfx_Scene_KeepXf(pItem, pcSoft->pcXf);
	pItem->Bounds = pcSoft->Opacity > 0 ? bounds : (WeUi_Gfx_ClipRect_t){0};
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Scene_Mask(WeUi_Gfx_Scene_t* pScene, const WeUi_Gfx_Path_t* pcPath, u8* pIdx) {
	if (pcPath->Broken)
		return RET_STATE_ERR_PARAM;

	WeUi_Gfx_SceneItem_t* pItem;
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Scene_New(pScene, WEUI_GFX_SCENE_MASK, pcPath, 0, &pItem, pIdx));
	WeUi_Gfx_Shape_t shape = WeUi_Gfx_Scene_Shape(pScene, pItem);
	return WeUi_Gfx_Raster_ShapeBounds(&shape, &pItem->Bounds);
}

RET_STATE_t WeUi_Gfx_Scene_Clip(WeUi_Gfx_Scene_t* pScene, u8 item, u8 mask,
								WEUI_GFX_MASK_MODE_t mode) {
	if (item >= pScene->ItemsNum || mask >= pScene->ItemsNum ||
		pScene->Items[mask].Kind != WEUI_GFX_SCENE_MASK ||
		pScene->Items[item].Kind == WEUI_GFX_SCENE_MASK)
		return RET_STATE_ERR_PARAM;

	WeUi_Gfx_SceneItem_t* pItem = &pScene->Items[item];
	if (pItem->ClipsNum >= WEUI_GFX_SCENE_ITEM_MASKS)
		return RET_STATE_ERR_OVERFLOW;

	pItem->Clips[pItem->ClipsNum++] = (WeUi_Gfx_SceneClip_t){.Mask = mask, .Mode = mode};
	return RET_STATE_SUCCESS;
}

// =================== Fingerprints ===================

static u32 WeUi_Gfx_Scene_Mix(u32 h, u32 v) {
	return (h ^ v) * WEUI_GFX_SCENE_FNV_PRIME;
}

static u32 WeUi_Gfx_Scene_Quant(float v, float scale) {
	return (u32)(s32)(v * scale + (v >= 0.0f ? WEUI_GFX_SCENE_ROUND : -WEUI_GFX_SCENE_ROUND));
}

static u32 WeUi_Gfx_Scene_MixPt(u32 h, WeUi_Gfx_PointF_t pt) {
	h = WeUi_Gfx_Scene_Mix(h, WeUi_Gfx_Scene_Quant(pt.X, WEUI_GFX_SCENE_QUANT_PX));
	return WeUi_Gfx_Scene_Mix(h, WeUi_Gfx_Scene_Quant(pt.Y, WEUI_GFX_SCENE_QUANT_PX));
}

static u32 WeUi_Gfx_Scene_MixXf(u32 h, const WeUi_Gfx_SceneItem_t* pcItem) {
	h = WeUi_Gfx_Scene_Mix(h, pcItem->HasXf);
	if (!pcItem->HasXf)
		return h;

	const WeUi_Gfx_Affine_t* pcXf = &pcItem->Xf;
	const float linear[]		  = {pcXf->A, pcXf->B, pcXf->C, pcXf->D};
	for (u32 i = 0; i < NUM_ELEMENTS(linear); i++)
		h = WeUi_Gfx_Scene_Mix(h, WeUi_Gfx_Scene_Quant(linear[i], WEUI_GFX_SCENE_QUANT_FINE));
	return WeUi_Gfx_Scene_MixPt(h, (WeUi_Gfx_PointF_t){pcXf->E, pcXf->F});
}

static u32 WeUi_Gfx_Scene_MixPaint(u32 h, const WeUi_Gfx_Scene_t* pcScene,
								   const WeUi_Gfx_SceneItem_t* pcItem) {
	const WeUi_Gfx_Paint_t* pcPaint = &pcItem->Paint;
	h = WeUi_Gfx_Scene_Mix(h, pcPaint->Type | (u32)pcPaint->Opacity << WEUI_GFX_SCENE_SHIFT);
	if (pcPaint->Type == WEUI_GFX_PAINT_SOLID)
		return WeUi_Gfx_Scene_Mix(h, WeUi_Gfx_Transform_Color2Bytes(pcPaint->Color));

	for (u32 i = 0; i < pcPaint->StopsNum; i++) {
		const WeUi_Gfx_PaintStop_t* pcStop = &pcScene->Stops[pcItem->Stop0 + i];
		h = WeUi_Gfx_Scene_Mix(h, WeUi_Gfx_Scene_Quant(pcStop->Offset, WEUI_GFX_SCENE_QUANT_FINE));
		h = WeUi_Gfx_Scene_Mix(h, WeUi_Gfx_Transform_Color2Bytes(pcStop->Color));
	}
	h = WeUi_Gfx_Scene_MixPt(h, pcPaint->P0);
	h = WeUi_Gfx_Scene_MixPt(h, pcPaint->P1);
	h = WeUi_Gfx_Scene_Mix(h, WeUi_Gfx_Scene_Quant(pcPaint->Radius, WEUI_GFX_SCENE_QUANT_PX));
	return WeUi_Gfx_Scene_MixXf(h, pcItem);
}

static u32 WeUi_Gfx_Scene_MixSoft(u32 h, const WeUi_Gfx_SceneItem_t* pcItem) {
	const WeUi_Gfx_Soft_t* pcSoft = &pcItem->Soft;
	h = WeUi_Gfx_Scene_Mix(h, WeUi_Gfx_Transform_Color2Bytes(pcSoft->Color));
	h = WeUi_Gfx_Scene_Mix(h, pcSoft->Opacity);
	h = WeUi_Gfx_Scene_MixPt(h, pcSoft->Centre);
	h = WeUi_Gfx_Scene_MixPt(h, (WeUi_Gfx_PointF_t){pcSoft->Rx, pcSoft->Ry});
	return WeUi_Gfx_Scene_MixXf(h, pcItem);
}

static u32 WeUi_Gfx_Scene_Avalanche(u32 h) {
	h ^= h >> WEUI_GFX_SCENE_FMIX_SHIFT1;
	h *= WEUI_GFX_SCENE_FMIX_MUL1;
	h ^= h >> WEUI_GFX_SCENE_FMIX_SHIFT2;
	h *= WEUI_GFX_SCENE_FMIX_MUL2;
	return h ^ (h >> WEUI_GFX_SCENE_FMIX_SHIFT1);
}

/* The points are summed, each hashed alone and mixed through: the fingerprint does not depend
 * on where a contour starts or on the order its points come in */
static u32 WeUi_Gfx_Scene_MixShape(u32 h, const WeUi_Gfx_Scene_t* pcScene,
								   const WeUi_Gfx_SceneItem_t* pcItem) {
	u32 sum = 0;
	for (u32 c = 0; c < pcItem->ContoursNum; c++) {
		const WeUi_Gfx_PathContour_t* pcC = &pcScene->Contours[pcItem->Contour0 + c];
		h								  = WeUi_Gfx_Scene_Mix(h, pcC->Num);
		for (u32 k = 0; k < pcC->Num; k++)
			sum += WeUi_Gfx_Scene_Avalanche(
				WeUi_Gfx_Scene_MixPt(WEUI_GFX_SCENE_FNV_BASIS, pcScene->Pts[pcC->Start + k]));
	}
	return WeUi_Gfx_Scene_Mix(h, sum);
}

static u32 WeUi_Gfx_Scene_Hash(const WeUi_Gfx_Scene_t* pcScene,
							   const WeUi_Gfx_SceneItem_t* pcItem) {
	u32 h = WeUi_Gfx_Scene_Mix(WEUI_GFX_SCENE_FNV_BASIS, pcItem->Kind);
	for (u32 i = 0; i < pcItem->ClipsNum; i++)
		h = WeUi_Gfx_Scene_Mix(h, pcItem->Clips[i].Mask | (u32)pcItem->Clips[i].Mode
															  << WEUI_GFX_SCENE_SHIFT);

	if (pcItem->Kind == WEUI_GFX_SCENE_SOFT)
		return WeUi_Gfx_Scene_MixSoft(h, pcItem);
	if (pcItem->Kind == WEUI_GFX_SCENE_FILL)
		h = WeUi_Gfx_Scene_MixPaint(h, pcScene, pcItem);
	return WeUi_Gfx_Scene_MixShape(h, pcScene, pcItem);
}

u8 WeUi_Gfx_Scene_Damage(WeUi_Gfx_Scene_t* pScene, WeUi_Gfx_ClipRect_t* pRects, u8 max) {
	WeUi_Gfx_ClipRect_t list[WEUI_GFX_SCENE_DAMAGE_KEPT];
	u32 num	   = 0;
	bool whole = !pScene->Shown || pScene->ItemsNum != pScene->PrevNum || pScene->Overflow;
	s32 gap	   = pScene->DamageGap;

	for (u32 i = 0; i < pScene->ItemsNum; i++) {
		const WeUi_Gfx_SceneItem_t* pcItem = &pScene->Items[i];
		u32 h							   = WeUi_Gfx_Scene_Hash(pScene, pcItem);
		if (!whole && h != pScene->PrevHash[i]) {
			WeUi_Gfx_Scene_AddRect(list, &num, pScene->PrevBounds[i], gap);
			WeUi_Gfx_Scene_AddRect(list, &num, pcItem->Bounds, gap);
		}
		pScene->PrevHash[i]	  = h;
		pScene->PrevBounds[i] = pcItem->Bounds;
	}
	pScene->PrevNum = pScene->ItemsNum;
	pScene->Shown	= true;

	WeUi_Gfx_ClipRect_t panel = WeUi_Gfx_Fill_PanelClip();
	if (whole) {
		list[0] = panel;
		num		= 1;
	}

	u8 out = 0;
	for (u32 i = 0; i < num && max > 0; i++) {
		WeUi_Gfx_ClipRect_t r = WeUi_Gfx_Scene_Align(list[i], pScene->DamageAlign);
		r					  = WeUi_Gfx_Scene_Cut(r, &panel);
		if (WeUi_Gfx_Scene_IsEmpty(&r))
			continue;
		if (out == max) {
			pRects[out - 1] = WeUi_Gfx_Scene_Join(pRects[out - 1], &r);
			continue;
		}
		pRects[out++] = r;
	}
	return out;
}

// =================== Drawing ===================

/** @brief The mask item rendered over rect, from the cache if it is there already. The slot
 *         `keep` holds another mask of the same item and is not given up for this one */
static RET_STATE_t WeUi_Gfx_Scene_CachedMask(const WeUi_Gfx_Scene_t* pcScene, u8 mask,
											 const WeUi_Gfx_ClipRect_t* pcRect, u32 keep,
											 u32* pSlot) {
	for (u32 k = 0; k < WEUI_GFX_SCENE_MASKS_CACHED; k++) {
		if (CacheItem[k] == mask) {
			*pSlot = k;
			return RET_STATE_SUCCESS;
		}
	}

	u32 k = CacheNext++ % WEUI_GFX_SCENE_MASKS_CACHED;
	if (k == keep)
		k = CacheNext++ % WEUI_GFX_SCENE_MASKS_CACHED;
	CacheItem[k] = WEUI_GFX_SCENE_NO_ITEM;
	RETURN_IF_UNSUCCESS(
		WeUi_Gfx_Mask_Init(&CacheMasks[k], CacheBufs[k], sizeof(CacheBufs[k]), pcRect));

	WeUi_Gfx_Shape_t shape = WeUi_Gfx_Scene_Shape(pcScene, &pcScene->Items[mask]);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Mask_AddShape(&CacheMasks[k], &shape, WEUI_GFX_PAINT_OPAQUE));
	CacheItem[k] = mask;
	*pSlot		 = k;
	return RET_STATE_SUCCESS;
}

/** @brief One item in one band. A mask kept inside that does not reach the band hides it */
static RET_STATE_t WeUi_Gfx_Scene_DrawItem(const WeUi_Gfx_Scene_t* pcScene,
										   const WeUi_Gfx_SceneItem_t* pcItem,
										   const WeUi_Gfx_ClipRect_t* pcBand,
										   const WeUi_Gfx_Target_t* pcTarget) {
	WeUi_Gfx_MaskUse_t uses[WEUI_GFX_SCENE_ITEM_MASKS];
	u8 usesNum = 0;
	u32 slot   = WEUI_GFX_SCENE_MASKS_CACHED;

	for (u32 i = 0; i < pcItem->ClipsNum; i++) {
		const WeUi_Gfx_SceneClip_t* pcClip = &pcItem->Clips[i];
		WeUi_Gfx_ClipRect_t r = WeUi_Gfx_Scene_Cut(pcScene->Items[pcClip->Mask].Bounds, pcBand);
		if (WeUi_Gfx_Scene_IsEmpty(&r)) {
			if (pcClip->Mode == WEUI_GFX_MASK_IN)
				return RET_STATE_SUCCESS;
			continue;
		}
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Scene_CachedMask(pcScene, pcClip->Mask, &r, slot, &slot));
		uses[usesNum++] = (WeUi_Gfx_MaskUse_t){.pcMask = &CacheMasks[slot], .Mode = pcClip->Mode};
	}

	const WeUi_Gfx_Affine_t* pcXf = pcItem->HasXf ? &pcItem->Xf : NULL;
	if (pcItem->Kind == WEUI_GFX_SCENE_SOFT) {
		WeUi_Gfx_Soft_t soft = pcItem->Soft;
		soft.pcXf			 = pcXf;
		soft.pcMasks		 = uses;
		soft.MasksNum		 = usesNum;
		return WeUi_Gfx_Fill_SoftTo(&soft, pcBand, pcTarget);
	}

	WeUi_Gfx_Paint_t paint = pcItem->Paint;
	paint.pcStops		   = &pcScene->Stops[pcItem->Stop0];
	paint.pcXf			   = pcXf;
	paint.pcMasks		   = uses;
	paint.MasksNum		   = usesNum;
	WeUi_Gfx_Shape_t shape = WeUi_Gfx_Scene_Shape(pcScene, pcItem);
	return WeUi_Gfx_Fill_PaintShapeTo(&shape, &paint, pcBand, pcTarget);
}

/** @brief Every item in the area, a band of rows at a time, into the target or the frame */
static RET_STATE_t WeUi_Gfx_Scene_DrawArea(const WeUi_Gfx_Scene_t* pcScene,
										   const WeUi_Gfx_ClipRect_t* pcArea,
										   const WeUi_Gfx_Target_t* pcTarget) {
	for (s32 by = pcArea->Y0; by < pcArea->Y1; by += WEUI_GFX_RASTER_BAND_ROWS) {
		WeUi_Gfx_ClipRect_t band = *pcArea;
		band.Y0					 = (WeUi_Gfx_PtPos_t)by;
		band.Y1 = (WeUi_Gfx_PtPos_t)GET_MIN(by + WEUI_GFX_RASTER_BAND_ROWS, (s32)pcArea->Y1);
		memset(CacheItem, WEUI_GFX_SCENE_NO_ITEM, sizeof(CacheItem));

		for (u32 i = 0; i < pcScene->ItemsNum; i++) {
			const WeUi_Gfx_SceneItem_t* pcItem = &pcScene->Items[i];
			WeUi_Gfx_ClipRect_t reach		   = WeUi_Gfx_Scene_Cut(pcItem->Bounds, &band);
			if (pcItem->Kind == WEUI_GFX_SCENE_MASK || WeUi_Gfx_Scene_IsEmpty(&reach))
				continue;
			RETURN_IF_UNSUCCESS(WeUi_Gfx_Scene_DrawItem(pcScene, pcItem, &band, pcTarget));
		}
	}
	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Scene_Draw(const WeUi_Gfx_Scene_t* pcScene,
								const WeUi_Gfx_ClipRect_t* pcClip) {
	WeUi_Gfx_ClipRect_t area = WeUi_Gfx_Fill_PanelClip();
	if (pcClip)
		area = WeUi_Gfx_Scene_Cut(area, pcClip);
	return WeUi_Gfx_Scene_DrawArea(pcScene, &area, NULL);
}

RET_STATE_t WeUi_Gfx_Scene_DrawTo(const WeUi_Gfx_Scene_t* pcScene,
								  const WeUi_Gfx_Target_t* pcTarget) {
	if (pcTarget->Rect.X1 - pcTarget->Rect.X0 > WEUI_GFX_RASTER_WIDTH_MAX)
		return RET_STATE_ERR_PARAM;
	return WeUi_Gfx_Scene_DrawArea(pcScene, &pcTarget->Rect, pcTarget);
}

#if WEUI_DISPLAY_BANDS
/* Band by band, each as many whole rows as a buffer holds, drawn and sent while the next is drawn */
static RET_STATE_t WeUi_Gfx_Scene_PresentRect(const WeUi_Gfx_Scene_t* pcScene,
											  const WeUi_Gfx_ClipRect_t* pcRect) {
	u32 w = (u32)(pcRect->X1 - pcRect->X0);
	for (s32 y = pcRect->Y0; y < pcRect->Y1;) {
		u32 pixels = 0;
		u16* pBuf  = WeUi_Gfx_Adapter_BandGet(&pixels);
		u32 rows   = pixels / w;
		rows -= rows % WEUI_DISPLAY_BAND_ALIGN;
		rows = GET_MIN(rows, (u32)(pcRect->Y1 - y));

		WeUi_Gfx_Target_t band = {
			.Format = WEUI_DISPLAY_BAND_FORMAT,
			.pBuf	= pBuf,
			.Stride = w,
			.Rect	= {pcRect->X0, (WeUi_Gfx_PtPos_t)y, pcRect->X1, (WeUi_Gfx_PtPos_t)(y + rows)},
		};
		/* The buffer goes back to the driver whatever happens to the band */
		RET_STATE_t rs = rows > 0 ? WeUi_Gfx_Scene_DrawTo(pcScene, &band) : RET_STATE_ERR_MEMORY;
		WeUi_Gfx_Adapter_BandSend((u16)band.Rect.X0, (u16)band.Rect.Y0, (u16)band.Rect.X1,
								  (u16)band.Rect.Y1, pBuf);
		RETURN_IF_UNSUCCESS(rs);
		y += (s32)rows;
	}
	return RET_STATE_SUCCESS;
}
#endif /* WEUI_DISPLAY_BANDS */

RET_STATE_t WeUi_Gfx_Scene_Present(const WeUi_Gfx_Scene_t* pcScene,
								   const WeUi_Gfx_ClipRect_t* pcRects, u8 num) {
#if WEUI_DISPLAY_BANDS
	for (u32 i = 0; i < num; i++)
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Scene_PresentRect(pcScene, &pcRects[i]));
#else
	/* The drawing frame holds anything after a push: the last picture first, then the changes */
	WeUi_Gfx_Adapter_CopyFrame_DisplayToDrawing();
	for (u32 i = 0; i < num; i++)
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Scene_Draw(pcScene, &pcRects[i]));
#endif /* WEUI_DISPLAY_BANDS */
	return RET_STATE_SUCCESS;
}
