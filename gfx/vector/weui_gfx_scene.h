#ifndef __WEUI_GFX_SCENE_H
#define __WEUI_GFX_SCENE_H

#include "weui_gfx_adapter.h"
#include "weui_gfx_fill.h"

/**
 * @file weui_gfx_scene.h
 * @brief A frame as a list of items - filled shapes, soft spots and masks, in the order they
 *        are drawn - kept so that the frame can be drawn again in any rectangle, and so that
 *        only what changed since the last frame has to be.
 *
 * A frame is built from scratch every time: Begin(), then the items. Each item keeps a copy of
 * what it needs - its shape's points, its gradient's stops, its transform - since the paths and
 * stacks it was built from are gone by the time it is drawn. Every item has a fingerprint of its
 * paint and points, the points summed so that their order does not count. Damage() compares the
 * fingerprints with the last frame's, item by item, and gives the rectangles where something
 * changed: the old place and the new one of each changed item, near ones merged. The first
 * frame, and one with a different number of items, is the whole panel.
 *
 * Draw() draws the items in a rectangle, a band of rows at a time, the masks an item is cut by
 * rendered for the band and cached for the items after it. Nothing is cleared first: a scene
 * whose first item covers the panel (a background) redraws every pixel of the rectangle.
 *
 * The scene is the caller's - one is a few kilobytes, sized by the settings below - and its
 * mask cache is static: one Draw() at a time.
 */

#ifndef WEUI_GFX_SCENE_ITEMS_MAX
#define WEUI_GFX_SCENE_ITEMS_MAX 32
#endif /* WEUI_GFX_SCENE_ITEMS_MAX */

/** @brief Points of all the items' shapes, flattened, 8 bytes each */
#ifndef WEUI_GFX_SCENE_PTS_MAX
#define WEUI_GFX_SCENE_PTS_MAX 512
#endif /* WEUI_GFX_SCENE_PTS_MAX */

#ifndef WEUI_GFX_SCENE_CONTOURS_MAX
#define WEUI_GFX_SCENE_CONTOURS_MAX 64
#endif /* WEUI_GFX_SCENE_CONTOURS_MAX */

/** @brief Stops of all the items' gradients */
#ifndef WEUI_GFX_SCENE_STOPS_MAX
#define WEUI_GFX_SCENE_STOPS_MAX 32
#endif /* WEUI_GFX_SCENE_STOPS_MAX */

/** @brief Masks a band keeps rendered: WEUI_GFX_RASTER_WIDTH_MAX x WEUI_GFX_RASTER_BAND_ROWS each */
#ifndef WEUI_GFX_SCENE_MASKS_CACHED
#define WEUI_GFX_SCENE_MASKS_CACHED 4
#endif /* WEUI_GFX_SCENE_MASKS_CACHED */

/**
 * @brief Rows of a mask cache slot: WEUI_GFX_RASTER_WIDTH_MAX x this many bytes each. A mask
 *        whose bounds fit a slot is rendered once for the whole frame; more rows than the
 *        raster band takes means smaller masks need not be re-rendered in every band.
 */
#ifndef WEUI_GFX_SCENE_MASK_ROWS
#define WEUI_GFX_SCENE_MASK_ROWS WEUI_GFX_RASTER_BAND_ROWS
#endif /* WEUI_GFX_SCENE_MASK_ROWS */

/** @brief Changed rectangles closer than this, in pixels, are given as one: a redraw, or a
 *         transfer to the panel, has a cost of its own */
#ifndef WEUI_GFX_SCENE_DAMAGE_GAP
#define WEUI_GFX_SCENE_DAMAGE_GAP 2
#endif /* WEUI_GFX_SCENE_DAMAGE_GAP */

/** @brief Changed rectangles start and end on multiples of this - 2 for a panel taking even
 *         windows only, as the CO5300 */
#ifndef WEUI_GFX_SCENE_DAMAGE_ALIGN
#define WEUI_GFX_SCENE_DAMAGE_ALIGN 1
#endif /* WEUI_GFX_SCENE_DAMAGE_ALIGN */

/** @brief Masks one item is cut by, all at once */
#define WEUI_GFX_SCENE_ITEM_MASKS 2

static_assert(WEUI_GFX_SCENE_ITEMS_MAX < UINT8_MAX, "items are numbered in a u8");
static_assert(WEUI_GFX_SCENE_MASKS_CACHED > WEUI_GFX_SCENE_ITEM_MASKS,
			  "an item's masks must be cached together");

typedef enum {
	WEUI_GFX_SCENE_FILL = 0,
	WEUI_GFX_SCENE_SOFT,
	WEUI_GFX_SCENE_MASK, /**< never drawn: cuts the items it is attached to */
} WEUI_GFX_SCENE_ITEM_t;

typedef struct {
	u8 Mask; /**< the mask item */
	WEUI_GFX_MASK_MODE_t Mode;
} WeUi_Gfx_SceneClip_t;

typedef struct {
	WEUI_GFX_SCENE_ITEM_t Kind;
	union {
		WeUi_Gfx_Paint_t Paint; /**< FILL: its pointers are the scene's at Draw() */
		WeUi_Gfx_Soft_t Soft;	/**< SOFT: likewise */
	};
	WeUi_Gfx_Affine_t Xf; /**< the paint's or the soft spot's transform, copied */
	bool HasXf;
	u16 Contour0; /**< FILL, MASK: the shape's contours in the scene's pool */
	u16 ContoursNum;
	u16 Stop0; /**< a gradient's stops in the scene's pool */
	WeUi_Gfx_SceneClip_t Clips[WEUI_GFX_SCENE_ITEM_MASKS];
	u8 ClipsNum;
	WeUi_Gfx_ClipRect_t Bounds; /**< the pixels it can reach; empty if it draws nothing */
} WeUi_Gfx_SceneItem_t;

typedef struct {
	WeUi_Gfx_SceneItem_t Items[WEUI_GFX_SCENE_ITEMS_MAX];
	WeUi_Gfx_PointF_t Pts[WEUI_GFX_SCENE_PTS_MAX];
	WeUi_Gfx_PathContour_t Contours[WEUI_GFX_SCENE_CONTOURS_MAX];
	WeUi_Gfx_PaintStop_t Stops[WEUI_GFX_SCENE_STOPS_MAX];
	u8 ItemsNum;
	u16 PtsNum;
	u16 ContoursNum;
	u16 StopsNum;
	bool Overflow; /**< an item did not fit: Damage() gives the whole panel */

	/* The last frame Damage() saw */
	u32 PrevHash[WEUI_GFX_SCENE_ITEMS_MAX];
	WeUi_Gfx_ClipRect_t PrevBounds[WEUI_GFX_SCENE_ITEMS_MAX];
	u8 PrevNum;
	bool Shown;

	u8 DamageGap;	/**< WEUI_GFX_SCENE_DAMAGE_GAP, to change after Init() */
	u8 DamageAlign; /**< WEUI_GFX_SCENE_DAMAGE_ALIGN, likewise; a power of two */
} WeUi_Gfx_Scene_t;

/** @brief An empty scene, nothing shown yet: the first Damage() is the whole panel */
void WeUi_Gfx_Scene_Init(WeUi_Gfx_Scene_t* pScene);

/** @brief Start a frame: the items go, the last frame's fingerprints stay for Damage() */
void WeUi_Gfx_Scene_Begin(WeUi_Gfx_Scene_t* pScene);

/** @brief The panel shows something else now - another scene, a transition's last frame: the
 *         next Damage() is the whole panel. The items stay, so a frame built already still goes */
void WeUi_Gfx_Scene_Forget(WeUi_Gfx_Scene_t* pScene);

/**
 * The builders below add an item at the end of the frame and give its number in *pIdx, which
 * may be NULL. They return RET_STATE_ERR_OVERFLOW when the scene has no room for it - the
 * scene is marked Overflow then - and RET_STATE_ERR_PARAM for what a fill would refuse, or a
 * paint or soft spot with masks of its own: in a scene masks are items, attached with Clip().
 * Either way nothing is added.
 */

/** @brief The path filled with the paint */
RET_STATE_t WeUi_Gfx_Scene_Fill(WeUi_Gfx_Scene_t* pScene, const WeUi_Gfx_Path_t* pcPath,
								const WeUi_Gfx_Paint_t* pcPaint, u8* pIdx);

RET_STATE_t WeUi_Gfx_Scene_Soft(WeUi_Gfx_Scene_t* pScene, const WeUi_Gfx_Soft_t* pcSoft, u8* pIdx);

/** @brief The path as a mask, wholly opaque, to Clip() items with */
RET_STATE_t WeUi_Gfx_Scene_Mask(WeUi_Gfx_Scene_t* pScene, const WeUi_Gfx_Path_t* pcPath, u8* pIdx);

/**
 * @brief Cut an item by a mask item: it shows only inside (IN) or outside (OUT) of it.
 * @return RET_STATE_ERR_PARAM for an item or a mask not in the frame, a mask that is not a
 *         MASK item or an item that is one; RET_STATE_ERR_OVERFLOW past
 *         WEUI_GFX_SCENE_ITEM_MASKS.
 */
RET_STATE_t WeUi_Gfx_Scene_Clip(WeUi_Gfx_Scene_t* pScene, u8 item, u8 mask,
								WEUI_GFX_MASK_MODE_t mode);

/**
 * @brief What changed since the last call, as rectangles on the panel to redraw - at most max,
 *        more merged into the last - and this frame becomes the last.
 * @return how many rectangles there are: 0 when nothing changed
 */
u8 WeUi_Gfx_Scene_Damage(WeUi_Gfx_Scene_t* pScene, WeUi_Gfx_ClipRect_t* pRects, u8 max);

/**
 * @brief Draw the frame's items into the drawing frame inside the clip (NULL: the panel), in
 *        their order.
 * @return the first error a fill returns; the items before it are drawn.
 */
RET_STATE_t WeUi_Gfx_Scene_Draw(const WeUi_Gfx_Scene_t* pcScene, const WeUi_Gfx_ClipRect_t* pcClip);

/**
 * @brief Draw the frame's items into the caller's buffer - every pixel of its Rect, in the
 *        scene's coordinates whatever the adapter's panel is: a band of a larger screen. Draw
 *        a screen band by band, each band a target of its own, while another goes out.
 * @return RET_STATE_ERR_PARAM for a Rect wider than WEUI_GFX_RASTER_WIDTH_MAX, and the first
 *         error a fill returns.
 */
RET_STATE_t WeUi_Gfx_Scene_DrawTo(const WeUi_Gfx_Scene_t* pcScene,
								  const WeUi_Gfx_Target_t* pcTarget);

/**
 * @brief Draw the whole frame into one of the adapter's frames, every pixel the scene covers:
 *        what a transition takes its pictures from. Through the frame as memory where the driver
 *        shows it (WEUI_DISPLAY_FRAME_MEM, 565 pixels), the rasteriser writing it straight;
 *        otherwise the drawing frame through the pixel calls, as Draw() does.
 * @return RET_STATE_ERR_PARAM for a frame neither way reaches - one the driver keeps only for a
 *         while, or the display frame of a driver that shows no frame as memory; the first error
 *         drawing returns.
 */
RET_STATE_t WeUi_Gfx_Scene_DrawToFrame(const WeUi_Gfx_Scene_t* pcScene, WEUI_GFX_FRAME_t frame);

/**
 * @brief Put the frame's changes on the panel - the rectangles Damage() gave - however the
 *        driver takes them. With a frame (WEUI_DISPLAY_BANDS 0) the last picture is copied back
 *        into the drawing frame and the rectangles drawn over it, to push as any frame is - by
 *        WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(), say - even when nothing changed. With bands each
 *        rectangle goes out a band at a time, through the driver's buffers, by the time this
 *        returns; the frame push does nothing then. The same code drives either panel.
 * @return the first error drawing returns; RET_STATE_ERR_MEMORY for a band buffer too small for
 *         one aligned row of a rectangle.
 */
RET_STATE_t WeUi_Gfx_Scene_Present(const WeUi_Gfx_Scene_t* pcScene,
								   const WeUi_Gfx_ClipRect_t* pcRects, u8 num);

/* TEMP profile: per-item and per-band draw times, host bench only */

#endif /* __WEUI_GFX_SCENE_H */
