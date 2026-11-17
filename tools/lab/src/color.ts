/** What a stored panel pixel means, in the units the library speaks */

export type Rgb = [number, number, number];

/** 0x00BBGGRR; a gray panel keeps its luma in the low byte alone */
export function decode(raw: number, gray: boolean): Rgb {
  const r = raw & 0xff;
  return gray ? [r, r, r] : [r, (raw >> 8) & 0xff, (raw >> 16) & 0xff];
}

/** RGBA for ImageData over a little-endian Uint32Array */
export function toRgba(raw: number, gray: boolean): number {
  if (!gray) return (raw | 0xff000000) >>> 0;
  const v = raw & 0xff;
  return (0xff000000 | (v << 16) | (v << 8) | v) >>> 0;
}

/** HSB as WeUi_ColorSchemeHsb_t holds it: hue 0..360, saturation and brightness 0..100 */
export function toHsb([r, g, b]: Rgb): Rgb {
  const max = Math.max(r, g, b);
  const d = max - Math.min(r, g, b);
  let h = 0;
  if (d) {
    if (max === r) h = ((g - b) / d) % 6;
    else if (max === g) h = (b - r) / d + 2;
    else h = (r - g) / d + 4;
    h = (h * 60 + 360) % 360;
  }
  return [h, max ? (d / max) * 100 : 0, (max / 255) * 100];
}

/** Luma weights of WeUi_Gfx_Transform_Color2Byte_Luma */
export function luma([r, g, b]: Rgb): number {
  return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

export const hex = (v: number, digits = 6) => "0x" + v.toString(16).toUpperCase().padStart(digits, "0");
