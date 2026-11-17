/*
  u8g2.c

  Simplified u8g2 - font parsing and rendering only.
  Merged from: u8x8_8x8.c, u8g2_intersection.c, u8g2_hvline.c, u8g2_setup.c, u8g2_font.c

  Original: Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list
    of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "u8g2.h"

/*========================================================================*/
/* UTF-8 decoder (from u8x8_8x8.c) */
/*========================================================================*/

void u8x8_utf8_init(u8x8_t* u8x8) {
	u8x8->utf8_state = 0;
}

u16 u8x8_utf8_next(u8x8_t* u8x8, u8 b) {
	if (b == 0 || b == '\n')
		return 0x0ffff;
	if (u8x8->utf8_state == 0) {
		if (b >= 0xfc) {
			u8x8->utf8_state = 5;
			b &= 1;
		} else if (b >= 0xf8) {
			u8x8->utf8_state = 4;
			b &= 3;
		} else if (b >= 0xf0) {
			u8x8->utf8_state = 3;
			b &= 7;
		} else if (b >= 0xe0) {
			u8x8->utf8_state = 2;
			b &= 15;
		} else if (b >= 0xc0) {
			u8x8->utf8_state = 1;
			b &= 0x01f;
		} else {
			return b;
		}
		u8x8->encoding = b;
		return 0x0fffe;
	} else {
		u8x8->utf8_state--;
		u8x8->encoding <<= 6;
		b &= 0x03f;
		u8x8->encoding |= b;
		if (u8x8->utf8_state != 0)
			return 0x0fffe;
	}
	return u8x8->encoding;
}

/*========================================================================*/
/* Intersection (from u8g2_intersection.c) */
/*========================================================================*/

#if defined(U8G2_WITH_INTERSECTION)

static u8 u8g2_is_intersection_decision_tree(u8g2_uint_t a0, u8g2_uint_t a1, u8g2_uint_t v0,
											 u8g2_uint_t v1) {
	if (v0 < a1) {
		if (v1 > a0) {
			return 1;
		} else {
			if (v0 > v1) {
				return 1;
			} else {
				return 0;
			}
		}
	} else {
		if (v1 > a0) {
			if (v0 > v1) {
				return 1;
			} else {
				return 0;
			}
		} else {
			return 0;
		}
	}
}

u8 u8g2_IsIntersection(u8g2_t* u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t x1,
					   u8g2_uint_t y1) {
	if (u8g2_is_intersection_decision_tree(u8g2->user_y0, u8g2->user_y1, y0, y1) == 0)
		return 0;
	return u8g2_is_intersection_decision_tree(u8g2->user_x0, u8g2->user_x1, x0, x1);
}

#endif /* U8G2_WITH_INTERSECTION */

/*========================================================================*/
/* HV line drawing and clipping (from u8g2_hvline.c) */
/*========================================================================*/

static u8 u8g2_clip_intersection2(u8g2_uint_t* ap, u8g2_uint_t* len, u8g2_uint_t c, u8g2_uint_t d) {
	u8g2_uint_t a = *ap;
	u8g2_uint_t b;
	b = a;
	b += *len;

	if (a > b) {
		if (a < d) {
			b = d;
			b--;
		} else {
			a = c;
		}
	}

	if (a >= d)
		return 0;
	if (b <= c)
		return 0;
	if (a < c)
		a = c;
	if (b > d)
		b = d;

	*ap = a;
	b -= a;
	*len = b;
	return 1;
}

static void u8g2_draw_hv_line_2dir(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len,
								   u8 dir) {
	y -= u8g2->pixel_curr_row;
	u8g2->ll_hvline(u8g2, x, y, len, dir);
}

void u8g2_DrawHVLine(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, u8 dir) {
	if (len != 0) {
		if (len > 1) {
			if (dir == 2) {
				x -= len;
				x++;
			} else if (dir == 3) {
				y -= len;
				y++;
			}
		}
		dir &= 1;

		if (dir == 0) {
			if (y < u8g2->user_y0)
				return;
			if (y >= u8g2->user_y1)
				return;
			if (u8g2_clip_intersection2(&x, &len, u8g2->user_x0, u8g2->user_x1) == 0)
				return;
		} else {
			if (x < u8g2->user_x0)
				return;
			if (x >= u8g2->user_x1)
				return;
			if (u8g2_clip_intersection2(&y, &len, u8g2->user_y0, u8g2->user_y1) == 0)
				return;
		}

		u8g2->cb->draw_l90(u8g2, x, y, len, dir);
	}
}

void u8g2_SetDrawColor(u8g2_t* u8g2, u8 color) {
	u8g2->draw_color = color;
	if (color >= 3)
		u8g2->draw_color = 1;
}

/*========================================================================*/
/* Setup (from u8g2_setup.c) */
/*========================================================================*/

static void u8g2_update_dimension_common(u8g2_t* u8g2) {
	const u8x8_display_info_t* display_info = u8g2_GetU8x8(u8g2)->display_info;
	u8g2_uint_t t;

	t = u8g2->tile_buf_height;
	t *= 8;
	u8g2->pixel_buf_height = t;

	t = display_info->tile_width;
	t *= 8;
	u8g2->pixel_buf_width = t;

	t = u8g2->tile_curr_row;
	t *= 8;
	u8g2->pixel_curr_row = t;

	t = u8g2->tile_buf_height;
	if (t + u8g2->tile_curr_row > display_info->tile_height)
		t = display_info->tile_height - u8g2->tile_curr_row;
	t *= 8;

	u8g2->buf_y0 = u8g2->pixel_curr_row;
	u8g2->buf_y1 = u8g2->buf_y0;
	u8g2->buf_y1 += t;

	u8g2->width	 = display_info->pixel_width;
	u8g2->height = display_info->pixel_height;
}

static void u8g2_update_dimension_r0(u8g2_t* u8g2) {
	u8g2_update_dimension_common(u8g2);
}

static void u8g2_update_page_win_r0(u8g2_t* u8g2) {
	u8g2->user_x0 = 0;
	u8g2->user_x1 = u8g2->width;
	u8g2->user_y0 = u8g2->buf_y0;
	u8g2->user_y1 = u8g2->buf_y1;
}

static void u8g2_draw_l90_r0(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, u8 dir) {
	u8g2_draw_hv_line_2dir(u8g2, x, y, len, dir);
}

const u8g2_cb_t u8g2_cb_r0 = {u8g2_update_dimension_r0, u8g2_update_page_win_r0, u8g2_draw_l90_r0};

/* Forward declaration */
static void u8g2_SetFontPosBaseline_internal(u8g2_t* u8g2);

void u8g2_SetupBuffer(u8g2_t* u8g2, u8* buf, u8 tile_buf_height,
					  u8g2_draw_ll_hvline_cb ll_hvline_cb, const u8g2_cb_t* u8g2_cb) {
	u8g2->font = NULL;

	u8g2->ll_hvline = ll_hvline_cb;

	u8g2->tile_buf_ptr	  = buf;
	u8g2->tile_buf_height = tile_buf_height;

	u8g2->tile_curr_row = 0;

	u8g2->font_decode.is_transparent = 0;
	u8g2->bitmap_transparency		 = 0;

	u8g2->font_height_mode	 = 0;
	u8g2->draw_color		 = 1;
	u8g2->is_auto_page_clear = 1;

	u8g2->cb = u8g2_cb;
	u8g2->cb->update_dimension(u8g2);
	u8g2->cb->update_page_win(u8g2);

	u8g2_SetFontPosBaseline_internal(u8g2);

#ifdef U8G2_WITH_FONT_ROTATION
	u8g2->font_decode.dir = 0;
#endif
}

/*========================================================================*/
/* Font rendering (from u8g2_font.c) */
/*========================================================================*/

#define U8G2_FONT_DATA_STRUCT_SIZE 23

/*--- Low level byte/word access ---*/

static u8 u8g2_font_get_byte(const u8* font, u8 offset) {
	font += offset;
	return u8g2_pgm_read(font);
}

static u16 u8g2_font_get_word(const u8* font, u8 offset) U8G2_NOINLINE;
static u16 u8g2_font_get_word(const u8* font, u8 offset) {
	u16 pos;
	font += offset;
	pos = u8g2_pgm_read(font);
	font++;
	pos <<= 8;
	pos += u8g2_pgm_read(font);
	return pos;
}

/*--- Font info ---*/

static void u8g2_read_font_info(u8g2_font_info_t* font_info, const u8* font) {
	font_info->glyph_cnt  = u8g2_font_get_byte(font, 0);
	font_info->bbx_mode	  = u8g2_font_get_byte(font, 1);
	font_info->bits_per_0 = u8g2_font_get_byte(font, 2);
	font_info->bits_per_1 = u8g2_font_get_byte(font, 3);

	font_info->bits_per_char_width	= u8g2_font_get_byte(font, 4);
	font_info->bits_per_char_height = u8g2_font_get_byte(font, 5);
	font_info->bits_per_char_x		= u8g2_font_get_byte(font, 6);
	font_info->bits_per_char_y		= u8g2_font_get_byte(font, 7);
	font_info->bits_per_delta_x		= u8g2_font_get_byte(font, 8);

	font_info->max_char_width  = u8g2_font_get_byte(font, 9);
	font_info->max_char_height = u8g2_font_get_byte(font, 10);
	font_info->x_offset		   = u8g2_font_get_byte(font, 11);
	font_info->y_offset		   = u8g2_font_get_byte(font, 12);

	font_info->ascent_A		= u8g2_font_get_byte(font, 13);
	font_info->descent_g	= u8g2_font_get_byte(font, 14);
	font_info->ascent_para	= u8g2_font_get_byte(font, 15);
	font_info->descent_para = u8g2_font_get_byte(font, 16);

	font_info->start_pos_upper_A = u8g2_font_get_word(font, 17);
	font_info->start_pos_lower_a = u8g2_font_get_word(font, 19);

#ifdef U8G2_WITH_UNICODE
	font_info->start_pos_unicode = u8g2_font_get_word(font, 21);
#endif
}

size_t u8g2_GetFontSize(const u8* font_arg) {
	u16 e;
	const u8* font = font_arg;
	font += U8G2_FONT_DATA_STRUCT_SIZE;

	for (;;) {
		if (u8g2_pgm_read(font + 1) == 0)
			break;
		font += u8g2_pgm_read(font + 1);
	}

	font += 2;
	font += u8g2_font_get_word(font, 0);

	for (;;) {
		e = u8g2_pgm_read(font);
		e <<= 8;
		e |= u8g2_pgm_read(font + 1);
		if (e == 0)
			break;
		font += u8g2_pgm_read(font + 2);
	}

	return (font - font_arg) + 2;
}

/*--- Glyph decoding ---*/

static u8 u8g2_font_decode_get_unsigned_bits(u8g2_font_decode_t* f, u8 cnt) {
	u8 val;
	u8 bit_pos = f->decode_bit_pos;
	u8 bit_pos_plus_cnt;

	val = u8g2_pgm_read(f->decode_ptr);

	val >>= bit_pos;
	bit_pos_plus_cnt = bit_pos;
	bit_pos_plus_cnt += cnt;
	if (bit_pos_plus_cnt >= 8) {
		u8 s = 8;
		s -= bit_pos;
		f->decode_ptr++;
		val |= u8g2_pgm_read(f->decode_ptr) << (s);
		bit_pos_plus_cnt -= 8;
	}
	val &= (1U << cnt) - 1;

	f->decode_bit_pos = bit_pos_plus_cnt;
	return val;
}

static s8 u8g2_font_decode_get_signed_bits(u8g2_font_decode_t* f, u8 cnt) {
	s8 v, d;
	v = (s8)u8g2_font_decode_get_unsigned_bits(f, cnt);
	d = 1;
	cnt--;
	d <<= cnt;
	v -= d;
	return v;
}

#ifdef U8G2_WITH_FONT_ROTATION
static u8g2_uint_t u8g2_add_vector_y(u8g2_uint_t dy, s8 x, s8 y, u8 dir) {
	switch (dir) {
		case 0:
			dy += y;
			break;
		case 1:
			dy += x;
			break;
		case 2:
			dy -= y;
			break;
		default:
			dy -= x;
			break;
	}
	return dy;
}

static u8g2_uint_t u8g2_add_vector_x(u8g2_uint_t dx, s8 x, s8 y, u8 dir) {
	switch (dir) {
		case 0:
			dx += x;
			break;
		case 1:
			dx -= y;
			break;
		case 2:
			dx -= x;
			break;
		default:
			dx += y;
			break;
	}
	return dx;
}
#endif

/*--- Glyph line drawing ---*/

static void u8g2_font_decode_len(u8g2_t* u8g2, u8 len, u8 is_foreground) {
	u8 cnt;
	u8 rem;
	u8 current;
	u8 lx, ly;
	u8g2_uint_t x, y;
	u8g2_font_decode_t* decode = &(u8g2->font_decode);

	cnt = len;
	lx	= decode->x;
	ly	= decode->y;

	for (;;) {
		rem = decode->glyph_width;
		rem -= lx;

		current = rem;
		if (cnt < rem)
			current = cnt;

		x = decode->target_x;
		y = decode->target_y;

#ifdef U8G2_WITH_FONT_ROTATION
		x = u8g2_add_vector_x(x, lx, ly, decode->dir);
		y = u8g2_add_vector_y(y, lx, ly, decode->dir);
#else
		x += lx;
		y += ly;
#endif

		if (is_foreground) {
			u8g2->draw_color = decode->fg_color;
			u8g2_DrawHVLine(u8g2, x, y, current,
#ifdef U8G2_WITH_FONT_ROTATION
							decode->dir
#else
							0
#endif
			);
		} else if (decode->is_transparent == 0) {
			u8g2->draw_color = decode->bg_color;
			u8g2_DrawHVLine(u8g2, x, y, current,
#ifdef U8G2_WITH_FONT_ROTATION
							decode->dir
#else
							0
#endif
			);
		}

		if (cnt < rem)
			break;
		cnt -= rem;
		lx = 0;
		ly++;
	}
	lx += cnt;

	decode->x = lx;
	decode->y = ly;
}

static void u8g2_font_setup_decode(u8g2_t* u8g2, const u8* glyph_data) {
	u8g2_font_decode_t* decode = &(u8g2->font_decode);
	decode->decode_ptr		   = glyph_data;
	decode->decode_bit_pos	   = 0;

	decode->glyph_width =
		u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_char_width);
	decode->glyph_height =
		u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_char_height);

	decode->fg_color = u8g2->draw_color;
	decode->bg_color = (decode->fg_color == 0 ? 1 : 0);
}

static s8 u8g2_font_decode_glyph(u8g2_t* u8g2, const u8* glyph_data) {
	u8 a, b;
	s8 x, y;
	s8 d;
	s8 h;
	u8g2_font_decode_t* decode = &(u8g2->font_decode);

	u8g2_font_setup_decode(u8g2, glyph_data);
	h = u8g2->font_decode.glyph_height;

	x = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_x);
	y = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_y);
	d = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_delta_x);

	if (decode->glyph_width > 0) {
#ifdef U8G2_WITH_FONT_ROTATION
		decode->target_x = u8g2_add_vector_x(decode->target_x, x, -(h + y), decode->dir);
		decode->target_y = u8g2_add_vector_y(decode->target_y, x, -(h + y), decode->dir);
#else
		decode->target_x += x;
		decode->target_y -= h + y;
#endif

#ifdef U8G2_WITH_INTERSECTION
		{
			u8g2_uint_t x0, x1, y0, y1;
			x0 = decode->target_x;
			y0 = decode->target_y;
			x1 = x0;
			y1 = y0;

#ifdef U8G2_WITH_FONT_ROTATION
			switch (decode->dir) {
				case 0:
					x1 += decode->glyph_width;
					y1 += h;
					break;
				case 1:
					x0 -= h;
					x0++;
					x1++;
					y1 += decode->glyph_width;
					break;
				case 2:
					x0 -= decode->glyph_width;
					x0++;
					x1++;
					y0 -= h;
					y0++;
					y1++;
					break;
				case 3:
					x1 += h;
					y0 -= decode->glyph_width;
					y0++;
					y1++;
					break;
				default:
					break;
			}
#else
			x1 += decode->glyph_width;
			y1 += h;
#endif

			if (u8g2_IsIntersection(u8g2, x0, y0, x1, y1) == 0)
				return d;
		}
#endif /* U8G2_WITH_INTERSECTION */

		decode->x = 0;
		decode->y = 0;

		for (;;) {
			a = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_0);
			b = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_1);
			do {
				u8g2_font_decode_len(u8g2, a, 0);
				u8g2_font_decode_len(u8g2, b, 1);
			} while (u8g2_font_decode_get_unsigned_bits(decode, 1) != 0);

			if (decode->y >= h)
				break;
		}

		u8g2->draw_color = decode->fg_color;
	}
	return d;
}

/*--- Glyph lookup ---*/

static const u8* u8g2_font_get_glyph_data(u8g2_t* u8g2, u16 encoding) {
	const u8* font = u8g2->font;
	font += U8G2_FONT_DATA_STRUCT_SIZE;

	if (encoding <= 255) {
		if (encoding >= 'a') {
			font += u8g2->font_info.start_pos_lower_a;
		} else if (encoding >= 'A') {
			font += u8g2->font_info.start_pos_upper_A;
		}

		for (;;) {
			if (u8g2_pgm_read(font + 1) == 0)
				break;
			if (u8g2_pgm_read(font) == encoding) {
				return font + 2;
			}
			font += u8g2_pgm_read(font + 1);
		}
	}
#ifdef U8G2_WITH_UNICODE
	else {
		u16 e;
		const u8* unicode_lookup_table;

		font += u8g2->font_info.start_pos_unicode;
		unicode_lookup_table = font;

		do {
			font += u8g2_font_get_word(unicode_lookup_table, 0);
			e = u8g2_font_get_word(unicode_lookup_table, 2);
			unicode_lookup_table += 4;
		} while (e < encoding);

		for (;;) {
			e = u8g2_pgm_read(font);
			e <<= 8;
			e |= u8g2_pgm_read(font + 1);

			if (e == 0)
				break;

			if (e == encoding) {
				return font + 3;
			}
			font += u8g2_pgm_read(font + 2);
		}
	}
#endif

	return NULL;
}

static u8g2_uint_t u8g2_font_draw_glyph(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u16 encoding) {
	u8g2_uint_t dx			   = 0;
	u8g2->font_decode.target_x = x;
	u8g2->font_decode.target_y = y;
	const u8* glyph_data	   = u8g2_font_get_glyph_data(u8g2, encoding);
	if (glyph_data != NULL) {
		dx = (u8g2_uint_t)(u8)u8g2_font_decode_glyph(u8g2, glyph_data);
	}
	return dx;
}

s8 u8g2_GetGlyphWidth(u8g2_t* u8g2, u16 requested_encoding) {
	const u8* glyph_data = u8g2_font_get_glyph_data(u8g2, requested_encoding);
	if (glyph_data == NULL)
		return 0;

	u8g2_font_setup_decode(u8g2, glyph_data);
	u8g2->glyph_x_offset =
		u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_x);
	u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_y);

	return u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_delta_x);
}

void u8g2_SetFontMode(u8g2_t* u8g2, u8 is_transparent) {
	u8g2->font_decode.is_transparent = is_transparent;
}

/*--- Glyph drawing (public) ---*/

u8g2_uint_t u8g2_DrawGlyph(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u16 encoding) {
#ifdef U8G2_WITH_FONT_ROTATION
	switch (u8g2->font_decode.dir) {
		case 0:
			y += u8g2->font_calc_vref(u8g2);
			break;
		case 1:
			x -= u8g2->font_calc_vref(u8g2);
			break;
		case 2:
			y -= u8g2->font_calc_vref(u8g2);
			break;
		case 3:
			x += u8g2->font_calc_vref(u8g2);
			break;
		default:
			break;
	}
#else
	y += u8g2->font_calc_vref(u8g2);
#endif
	return u8g2_font_draw_glyph(u8g2, x, y, encoding);
}

/*--- String drawing ---*/

static u8g2_uint_t u8g2_draw_string(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y,
									const char* str) U8G2_NOINLINE;
static u8g2_uint_t u8g2_draw_string(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, const char* str) {
	u16 e;
	u8g2_uint_t delta, sum;
	u8x8_utf8_init(u8g2_GetU8x8(u8g2));
	sum = 0;
	for (;;) {
		e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (u8)*str);
		if (e == 0x0ffff)
			break;
		str++;
		if (e != 0x0fffe) {
			delta = u8g2_DrawGlyph(u8g2, x, y, e);

#ifdef U8G2_WITH_FONT_ROTATION
			switch (u8g2->font_decode.dir) {
				case 0:
					x += delta;
					break;
				case 1:
					y += delta;
					break;
				case 2:
					x -= delta;
					break;
				case 3:
					y -= delta;
					break;
				default:
					break;
			}
#else
			x += delta;
#endif

			sum += delta;
		}
	}
	return sum;
}

u8g2_uint_t u8g2_DrawUTF8(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, const char* str) {
	u8g2->u8x8.next_cb = u8x8_utf8_next;
	return u8g2_draw_string(u8g2, x, y, str);
}

/*--- Font ref height ---*/

static void u8g2_UpdateRefHeight(u8g2_t* u8g2) {
	if (u8g2->font == NULL)
		return;
	u8g2->font_ref_ascent  = u8g2->font_info.ascent_A;
	u8g2->font_ref_descent = u8g2->font_info.descent_g;
	if (u8g2->font_height_mode == U8G2_FONT_HEIGHT_MODE_TEXT) {
	} else if (u8g2->font_height_mode == U8G2_FONT_HEIGHT_MODE_XTEXT) {
		if (u8g2->font_ref_ascent < u8g2->font_info.ascent_para)
			u8g2->font_ref_ascent = u8g2->font_info.ascent_para;
		if (u8g2->font_ref_descent > u8g2->font_info.descent_para)
			u8g2->font_ref_descent = u8g2->font_info.descent_para;
	} else {
		if (u8g2->font_ref_ascent < u8g2->font_info.max_char_height + u8g2->font_info.y_offset)
			u8g2->font_ref_ascent = u8g2->font_info.max_char_height + u8g2->font_info.y_offset;
		if (u8g2->font_ref_descent > u8g2->font_info.y_offset)
			u8g2->font_ref_descent = u8g2->font_info.y_offset;
	}
}

static u8g2_uint_t u8g2_font_calc_vref_font(U8G2_UNUSED u8g2_t* u8g2) {
	return 0;
}

static void u8g2_SetFontPosBaseline_internal(u8g2_t* u8g2) {
	u8g2->font_calc_vref = u8g2_font_calc_vref_font;
}

void u8g2_SetFontPosBaseline(u8g2_t* u8g2) {
	u8g2->font_calc_vref = u8g2_font_calc_vref_font;
}

/*--- Font selection ---*/

void u8g2_SetFont(u8g2_t* u8g2, const u8* font) {
	if (u8g2->font != font) {
		u8g2->font = font;
		u8g2_read_font_info(&(u8g2->font_info), font);
		u8g2_UpdateRefHeight(u8g2);
	}
}

/*--- String width ---*/

static u8g2_uint_t u8g2_string_width(u8g2_t* u8g2, const char* str) U8G2_NOINLINE;
static u8g2_uint_t u8g2_string_width(u8g2_t* u8g2, const char* str) {
	u16 e;
	u8g2_uint_t w, dx;
#ifdef U8G2_BALANCED_STR_WIDTH_CALCULATION
	s8 initial_x_offset = -64;
#endif

	u8g2->font_decode.glyph_width = 0;
	u8x8_utf8_init(u8g2_GetU8x8(u8g2));

	w  = 0;
	dx = 0;

	for (;;) {
		e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (u8)*str);
		if (e == 0x0ffff)
			break;
		str++;
		if (e != 0x0fffe) {
			dx = (u8g2_uint_t)(u8)u8g2_GetGlyphWidth(u8g2, e);
#ifdef U8G2_BALANCED_STR_WIDTH_CALCULATION
			if (initial_x_offset == -64)
				initial_x_offset = u8g2->glyph_x_offset;
#endif
			w += dx;
		}
	}

	if (u8g2->font_decode.glyph_width != 0) {
		w -= dx;
		w += u8g2->font_decode.glyph_width;
		w += u8g2->glyph_x_offset;
#ifdef U8G2_BALANCED_STR_WIDTH_CALCULATION
		if (initial_x_offset > 0)
			w += initial_x_offset;
#endif
	}

	return w;
}

u8g2_uint_t u8g2_GetUTF8Width(u8g2_t* u8g2, const char* str) {
	u8g2->u8x8.next_cb = u8x8_utf8_next;
	return u8g2_string_width(u8g2, str);
}

void u8g2_SetFontDirection(u8g2_t* u8g2, u8 dir) {
#ifdef U8G2_WITH_FONT_ROTATION
	u8g2->font_decode.dir = dir;
#endif
}
