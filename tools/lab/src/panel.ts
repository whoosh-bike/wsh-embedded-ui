import { toRgba } from "./color";

/**
 * The panel, zoomed. "led" draws every pixel as a round dot with a dark gap, as the
 * matrix looks; "flat" as square pixels, optionally with a grid. Diff dims what the
 * previous frame already had, so only what moved stands out.
 */

export type Look = "led" | "flat";

export interface PanelView {
  look: Look;
  grid: boolean;
  diff: boolean;
  zoom: number;
}

const GAP_COLOR = "#0e0e13";
const GRID_COLOR = "rgba(255,255,255,0.08)";
const PIN_COLOR = "#a0c4ff";
const DIFF_KEEP = 0.18;
const MIN_ZOOM_FOR_GRID = 4;

export class Panel {
  readonly canvas: HTMLCanvasElement;
  private readonly ctx: CanvasRenderingContext2D;
  private readonly src = document.createElement("canvas");
  private readonly srcCtx = this.src.getContext("2d")!;
  private mask: HTMLCanvasElement | null = null;
  private maskKey = "";
  private image: ImageData | null = null;
  private width = 0;
  private height = 0;

  constructor(canvas: HTMLCanvasElement) {
    this.canvas = canvas;
    this.ctx = canvas.getContext("2d")!;
  }

  draw(
    frame: Uint32Array | null,
    prev: Uint32Array | null,
    width: number,
    height: number,
    gray: boolean,
    view: PanelView,
    pin: { x: number; y: number } | null,
  ) {
    const z = view.zoom;
    if (this.width !== width || this.height !== height || !this.image) {
      this.width = width;
      this.height = height;
      this.src.width = width;
      this.src.height = height;
      this.image = this.srcCtx.createImageData(width, height);
    }
    if (this.canvas.width !== width * z || this.canvas.height !== height * z) {
      this.canvas.width = width * z;
      this.canvas.height = height * z;
    }

    const out = new Uint32Array(this.image.data.buffer);
    if (!frame) out.fill(0xff000000);
    else
      for (let i = 0; i < out.length; i++) {
        let c = toRgba(frame[i], gray);
        if (view.diff && prev && prev[i] === frame[i]) c = dim(c, DIFF_KEEP);
        out[i] = c;
      }
    this.srcCtx.putImageData(this.image, 0, 0);

    const ctx = this.ctx;
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(this.src, 0, 0, width * z, height * z);

    if (view.look === "led" && z >= MIN_ZOOM_FOR_GRID) ctx.drawImage(this.ledMask(width, height, z), 0, 0);
    else if (view.grid && z >= MIN_ZOOM_FOR_GRID) this.drawGrid(width, height, z);

    if (pin) {
      ctx.strokeStyle = PIN_COLOR;
      ctx.lineWidth = Math.max(1, z / 8);
      ctx.strokeRect(pin.x * z + 0.5, pin.y * z + 0.5, z - 1, z - 1);
    }
  }

  /** Panel pixel under a mouse event, or null off the panel */
  pixelAt(e: MouseEvent, zoom: number): { x: number; y: number } | null {
    const r = this.canvas.getBoundingClientRect();
    const x = Math.floor(((e.clientX - r.left) / r.width) * this.canvas.width / zoom);
    const y = Math.floor(((e.clientY - r.top) / r.height) * this.canvas.height / zoom);
    return x >= 0 && y >= 0 && x < this.width && y < this.height ? { x, y } : null;
  }

  private drawGrid(width: number, height: number, z: number) {
    const ctx = this.ctx;
    ctx.strokeStyle = GRID_COLOR;
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let x = 1; x < width; x++) ctx.moveTo(x * z + 0.5, 0), ctx.lineTo(x * z + 0.5, height * z);
    for (let y = 1; y < height; y++) ctx.moveTo(0, y * z + 0.5), ctx.lineTo(width * z, y * z + 0.5);
    ctx.stroke();
  }

  /** Dark sheet with a round hole per pixel, built once per size */
  private ledMask(width: number, height: number, z: number): HTMLCanvasElement {
    const key = `${width}x${height}@${z}`;
    if (this.mask && this.maskKey === key) return this.mask;

    const mask = document.createElement("canvas");
    mask.width = width * z;
    mask.height = height * z;
    const m = mask.getContext("2d")!;
    m.fillStyle = GAP_COLOR;
    m.fillRect(0, 0, mask.width, mask.height);
    m.globalCompositeOperation = "destination-out";
    const r = z * 0.42;
    m.beginPath();
    for (let y = 0; y < height; y++)
      for (let x = 0; x < width; x++) {
        m.moveTo(x * z + z / 2 + r, y * z + z / 2);
        m.arc(x * z + z / 2, y * z + z / 2, r, 0, Math.PI * 2);
      }
    m.fill();

    this.mask = mask;
    this.maskKey = key;
    return mask;
  }
}

function dim(rgba: number, k: number): number {
  const r = (rgba & 0xff) * k, g = ((rgba >> 8) & 0xff) * k, b = ((rgba >> 16) & 0xff) * k;
  return (0xff000000 | (b << 16) | (g << 8) | r) >>> 0;
}
