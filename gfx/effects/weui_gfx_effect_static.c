#include "weui_gfx_effect_static.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_transform.h"
#include "weui_rand.h"

void WeUi_Gfx_Effect_Static_Paint(void) {
	for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)WeUi_Gfx_Adapter_GetWidth(); x++) {
		for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)WeUi_Gfx_Adapter_GetHeight(); y++) {
			WeUi_Gfx_ColorSchemeHsb_t hsbColor = WeUi_Color_GetFullHsb(WeUi_Rand_Hue());

			WeUi_Gfx_ColorPoint_t cpt = {
				.Pt.X  = x,
				.Pt.Y  = y,
				.Color = WeUi_Rand_Bool() ? WeUi_Gfx_Color_FromHsb(&hsbColor)
										  : WeUi_Gfx_Transform_FillColorFromChannel(
												WeUi_Gfx_Adapter_GetColor_Min()),
			};

			WeUi_Gfx_Point_Draw(&cpt);
		}
	}
}
