#ifndef __WEUI_GFX_FILL_H
#define __WEUI_GFX_FILL_H

#include "weui_gfx_mask.h"

/**
 * @file weui_gfx_fill.h
 * @brief A path painted into the drawing frame - one colour or a gradient, at an opacity - its
 *        edges blended with what is drawn there; and the soft spot, an ellipse fading out
 *        from its centre with no path at all.
 *
 * Blending happens on pixels as the driver stores them: a driver that scales brightness has
 * scaled what is in the frame already, so the paint's colours are first taken through the
 * driver - once per fill, into a table for a gradient - and the stored values are mixed.
 * Running a blend through SetPixel would scale the old picture a second time.
 *
 * Like the rasteriser underneath, one fill at a time.
 */

/** @brief Opacity of a paint drawn as it is */
#define WEUI_GFX_PAINT_OPAQUE 255

/**
 * @brief Gradients whose colours for a buffer are kept, 1.5 KB each: a scene of several drawn
 *        band by band works one out again whenever it follows another in a band
 */
#ifndef WEUI_GFX_FILL_LUTS
#define WEUI_GFX_FILL_LUTS 1
#endif /* WEUI_GFX_FILL_LUTS */

typedef enum {
	WEUI_GFX_PAINT_SOLID = 0,
	WEUI_GFX_PAINT_LINEAR, /**< along the line from P0 to P1, the same across it */
	WEUI_GFX_PAINT_RADIAL, /**< in rings round P0, out to Radius */
} WEUI_GFX_PAINT_t;

/** @brief A colour of a gradient at Offset in [0, 1] of its way */
typedef struct {
	float Offset;
	WeUi_Gfx_Color_t Color;
} WeUi_Gfx_PaintStop_t;

/**
 * @brief What fills a shape. Before the first stop's offset a gradient has its colour, past
 *        the last one the last colour.
 */
typedef struct {
	WEUI_GFX_PAINT_t Type;
	WeUi_Gfx_Color_t Color;				 /**< SOLID */
	const WeUi_Gfx_PaintStop_t* pcStops; /**< gradients: offsets ascending */
	u8 StopsNum;
	WeUi_Gfx_PointF_t P0; /**< LINEAR: offset 0; RADIAL: the centre */
	WeUi_Gfx_PointF_t P1; /**< LINEAR: offset 1 */
	float Radius;		  /**< RADIAL: offset 1 at this distance from P0 */
	/**
	 * The coordinates P0, P1 and Radius are in, onto the panel - the shape's own, so the
	 * gradient turns and stretches with it; read once per fill. NULL: panel coordinates.
	 */
	const WeUi_Gfx_Affine_t* pcXf;
	u8 Opacity; /**< WEUI_GFX_PAINT_OPAQUE for none */
	/** Masks the paint keeps inside or out of, all of them at once; none when MasksNum is 0 */
	const WeUi_Gfx_MaskUse_t* pcMasks;
	u8 MasksNum;
} WeUi_Gfx_Paint_t;

/** @brief A spot fading out from its centre: coverage (1 - r^2)^2 over the ellipse */
typedef struct {
	WeUi_Gfx_PointF_t Centre;
	float Rx;
	float Ry;
	WeUi_Gfx_Color_t Color;
	const WeUi_Gfx_Affine_t* pcXf; /**< as WeUi_Gfx_Paint_t's */
	u8 Opacity;
	const WeUi_Gfx_MaskUse_t* pcMasks; /**< as WeUi_Gfx_Paint_t's */
	u8 MasksNum;
} WeUi_Gfx_Soft_t;

typedef enum {
	WEUI_GFX_TARGET_RGB565 = 0, /**< 16-bit pixels, red in the top bits */
	WEUI_GFX_TARGET_RGB565_BE,	/**< the same, high byte first, as SPI panels take them */
} WEUI_GFX_TARGET_t;

/**
 * @brief A buffer of the caller's to fill in place of the drawing frame: a band to send to
 *        the panel, say. It takes colours as they are - brightness is the panel's own then - and
 *        a fill is clipped to the pixels it holds.
 */
typedef struct {
	WEUI_GFX_TARGET_t Format;
	u16* pBuf;
	u32 Stride;				  /**< pixels from the start of one row to the next */
	WeUi_Gfx_ClipRect_t Rect; /**< the panel pixels it holds; pBuf[0] is (Rect.X0, Rect.Y0) */
} WeUi_Gfx_Target_t;

/** @brief The whole panel, as a clip */
WeUi_Gfx_ClipRect_t WeUi_Gfx_Fill_PanelClip(void);

/** @brief One colour, opaque */
WeUi_Gfx_Paint_t WeUi_Gfx_Paint_Solid(WeUi_Gfx_Color_t color);

/** @brief RET_STATE_ERR_PARAM where WeUi_Gfx_Fill_Paint() would refuse the paint itself */
RET_STATE_t WeUi_Gfx_Paint_Check(const WeUi_Gfx_Paint_t* pcPaint);

/**
 * @brief Fill the path with the paint, inside the clip - NULL for the whole panel; a clip
 *        reaching off the panel is cut to it.
 * @return RET_STATE_ERR_PARAM for a gradient with no stops or with them out of order, a linear
 *         one with P0 = P1, a radial one with no radius, a pcXf that flattens the plane, or a
 *         mask missing - and whatever WeUi_Gfx_Raster_Fill() refuses. Nothing is drawn on an
 *         error.
 */
RET_STATE_t WeUi_Gfx_Fill_Paint(const WeUi_Gfx_Path_t* pcPath, const WeUi_Gfx_Paint_t* pcPaint,
								const WeUi_Gfx_ClipRect_t* pcClip);

/** @brief WeUi_Gfx_Fill_Paint() of a shape, which cannot be Broken */
RET_STATE_t WeUi_Gfx_Fill_PaintShape(const WeUi_Gfx_Shape_t* pcShape,
									 const WeUi_Gfx_Paint_t* pcPaint,
									 const WeUi_Gfx_ClipRect_t* pcClip);

/**
 * @brief WeUi_Gfx_Fill_PaintShape() into a buffer (NULL: the drawing frame), clipped to the
 *        pixels it holds and to pcClip as well when that is not NULL.
 * @return RET_STATE_ERR_PARAM as WeUi_Gfx_Fill_Paint(), and for a target with no buffer or
 *         rows shorter than its Rect
 */
RET_STATE_t WeUi_Gfx_Fill_PaintShapeTo(const WeUi_Gfx_Shape_t* pcShape,
									   const WeUi_Gfx_Paint_t* pcPaint,
									   const WeUi_Gfx_ClipRect_t* pcClip,
									   const WeUi_Gfx_Target_t* pcTarget);

/** @brief WeUi_Gfx_Fill_Paint() with WeUi_Gfx_Paint_Solid(color) */
RET_STATE_t WeUi_Gfx_Fill_Solid(const WeUi_Gfx_Path_t* pcPath, WeUi_Gfx_Color_t color,
								const WeUi_Gfx_ClipRect_t* pcClip);

/**
 * @brief The pixels the soft spot can reach, as WeUi_Gfx_Raster_Bounds() gives a path's.
 * @return RET_STATE_ERR_PARAM for a radius not above 0.
 */
RET_STATE_t WeUi_Gfx_Fill_SoftBounds(const WeUi_Gfx_Soft_t* pcSoft, WeUi_Gfx_ClipRect_t* pBounds);

/**
 * @brief Blend the soft spot in, inside the clip as for a fill.
 * @return RET_STATE_ERR_PARAM for a radius not above 0, a pcXf with no inverse or a mask
 *         missing.
 */
RET_STATE_t WeUi_Gfx_Fill_Soft(const WeUi_Gfx_Soft_t* pcSoft, const WeUi_Gfx_ClipRect_t* pcClip);

/** @brief WeUi_Gfx_Fill_Soft() into a buffer, as WeUi_Gfx_Fill_PaintShapeTo() */
RET_STATE_t WeUi_Gfx_Fill_SoftTo(const WeUi_Gfx_Soft_t* pcSoft, const WeUi_Gfx_ClipRect_t* pcClip,
								 const WeUi_Gfx_Target_t* pcTarget);

#endif /* __WEUI_GFX_FILL_H */
