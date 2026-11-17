#include "weui_gfx_effect_zoom.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* Below this the picture is a dot or less: the old one is left whole */
#define WEUI_GFX_EFFECT_ZOOM_SCALE_MIN 0.02f

/**
 * @brief Shrink one line of the drawing frame towards its centre, in place.
 *
 * Shrinking, every pixel reads one further out than itself. Walked from the centre
 * outwards, what it reads is not written yet - so no copy of the frame is needed.
 * A row, or a column when vertical: the 2D shrink is one pass of each.
 */
static void WeUi_Gfx_Effect_ZoomLine(bool vertical, WeUi_Gfx_PtPos_t line, WeUi_Gfx_PtPos_t span,
									 float scale) {
	float centre = ((float)span - 1.0f) / 2.0f;
	float reach	 = (float)span / 2.0f * scale;

	for (WeUi_Gfx_PtPos_t step = 0; step <= span / 2; step++) {
		WeUi_Gfx_PtPos_t sides[] = {(WeUi_Gfx_PtPos_t)(centre + 0.5f) + step,
									(WeUi_Gfx_PtPos_t)centre - step};

		for (u32 side = 0; side < NUM_ELEMENTS(sides); side++) {
			WeUi_Gfx_PtPos_t pos = sides[side];
			float off			 = (float)pos - centre;

			if (pos < 0 || pos >= span || off > reach || -off > reach)
				continue;

			WeUi_Gfx_PtPos_t src = (WeUi_Gfx_PtPos_t)(centre + off / scale + 0.5f);
			if (src < 0)
				src = 0;
			if (src >= span)
				src = span - 1;

			WeUi_Gfx_Point_t dst  = vertical ? (WeUi_Gfx_Point_t){.X = line, .Y = pos}
											 : (WeUi_Gfx_Point_t){.X = pos, .Y = line};
			WeUi_Gfx_Point_t from = vertical ? (WeUi_Gfx_Point_t){.X = line, .Y = src}
											 : (WeUi_Gfx_Point_t){.X = src, .Y = line};
			WeUi_Gfx_Adapter_SetPixelRaw(&dst, WeUi_Gfx_Adapter_GetPixelRaw(&from));
		}
	}
}

static void WeUi_Gfx_Effect_Compose_Zoom(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	WeUi_Gfx_PtPos_t width	= (WeUi_Gfx_PtPos_t)pcCtx->Width;
	WeUi_Gfx_PtPos_t height = (WeUi_Gfx_PtPos_t)pcCtx->Height;
	/* It swells fast through the middle and settles onto the edges */
	float scale				= WeUi_Gfx_Effect_Smooth(pcCtx->Progress);

	if (scale >= 1.0f)
		return;

	if (scale >= WEUI_GFX_EFFECT_ZOOM_SCALE_MIN) {
		for (WeUi_Gfx_PtPos_t y = 0; y < height; y++)
			WeUi_Gfx_Effect_ZoomLine(false, y, width, scale);
		for (WeUi_Gfx_PtPos_t x = 0; x < width; x++)
			WeUi_Gfx_Effect_ZoomLine(true, x, height, scale);
	}

	/* Around the shrunk picture the old one stays */
	float halfW = (float)width / 2.0f * scale;
	float halfH = (float)height / 2.0f * scale;
	float cx	= ((float)width - 1.0f) / 2.0f;
	float cy	= ((float)height - 1.0f) / 2.0f;

	for (WeUi_Gfx_PtPos_t y = 0; y < height; y++) {
		for (WeUi_Gfx_PtPos_t x = 0; x < width; x++) {
			bool inside = scale >= WEUI_GFX_EFFECT_ZOOM_SCALE_MIN && (float)x - cx <= halfW &&
						  cx - (float)x <= halfW && (float)y - cy <= halfH &&
						  cy - (float)y <= halfH;
			if (inside)
				continue;

			WeUi_Gfx_Effect_PutOld(x, y, x, y);
		}
	}
}

RET_STATE_t WeUi_Gfx_Effect_Zoom(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Zoom, frames, NULL,
										 renderIncoming);
}
