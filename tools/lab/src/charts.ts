import { LOG_LEVELS } from "./protocol";
import type { Recording } from "./recording";
import type { SceneModel } from "./scenes";

/**
 * The strips under the panel, all a run wide in virtual time and seekable by click: the
 * scenes (which was up, what played over it, where an event came in), the timeline
 * (effect spans, log lines, seconds) and the history of the pinned pixel - its channels
 * over every frame, the curve a fade or a spring actually drew. Time, not frames: a scene
 * that holds still for ten seconds pushes one frame, and still takes ten seconds of strip.
 */

const EFFECT_COLOR = "rgba(160, 196, 255, 0.22)";
const PANIC_COLOR = "#ff6b7d";
const HEAD_COLOR = "#a0c4ff";
const WARN_COLOR = "#f59e0b";
const TICK_COLOR = "rgba(255,255,255,0.18)";
const LABEL_COLOR = "rgba(255,255,255,0.45)";
const LOOP_COLOR = "rgba(245, 158, 11, 0.14)";
const CHANNEL_COLORS = ["#ff6b6b", "#5fd38d", "#6ea8fe"];
const GRAY_COLOR = "#d9d9d9";
const MS_PER_S = 1000;
const LABEL_GAP_PX = 36;
const SECOND_STEPS = [1, 2, 5, 10, 15, 30, 60, 120, 300, 600];
const LOG_TICK_COLOR = "rgba(255,255,255,0.35)";
const LOG_WARN = LOG_LEVELS.indexOf("WARN");
const LOG_ERROR = LOG_LEVELS.indexOf("ERROR");

const SCENE_ROW_SHARE = 0.62;
const SCENE_TEXT_COLOR = "rgba(255,255,255,0.88)";
const SCENE_EDGE_COLOR = "rgba(0,0,0,0.55)";
const TRANSITION_COLOR = "#a0c4ff";
const ACTION_COLOR = "#f59e0b";
const PLAY_TEXT_COLOR = "#15151c";
const MARK_COLOR = "rgba(255,255,255,0.8)";
const MARK_PX = 4;
const LABEL_PAD_PX = 4;
const GOLDEN_ANGLE = 137.5;

/** A scene's colour, by index: neighbours in the table far apart on the wheel */
export const sceneColor = (scene: number, alpha = 1) => `hsl(${(scene * GOLDEN_ANGLE) % 360} 42% 34% / ${alpha})`;

function fit(canvas: HTMLCanvasElement): CanvasRenderingContext2D {
  const ratio = window.devicePixelRatio || 1;
  const w = Math.round(canvas.clientWidth * ratio), h = Math.round(canvas.clientHeight * ratio);
  if (canvas.width !== w || canvas.height !== h) {
    canvas.width = w;
    canvas.height = h;
  }
  const ctx = canvas.getContext("2d")!;
  ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
  ctx.clearRect(0, 0, canvas.clientWidth, canvas.clientHeight);
  return ctx;
}

/** Virtual time under the mouse on a run-wide strip */
export function timeFromClick(e: MouseEvent, canvas: HTMLCanvasElement, end: number): number {
  const r = canvas.getBoundingClientRect();
  return Math.max(0, Math.min(end, ((e.clientX - r.left) / r.width) * end));
}

function drawHead(ctx: CanvasRenderingContext2D, x: number, h: number) {
  ctx.fillStyle = HEAD_COLOR;
  ctx.fillRect(x - 1, 0, 2, h);
}

/** Text in a span when it fits, clipped to it */
function spanLabel(ctx: CanvasRenderingContext2D, text: string, x0: number, x1: number, y: number, color: string) {
  if (x1 - x0 < ctx.measureText(text).width + 2 * LABEL_PAD_PX) return;
  ctx.fillStyle = color;
  ctx.fillText(text, x0 + LABEL_PAD_PX, y);
}

export function drawScenes(canvas: HTMLCanvasElement, model: SceneModel | null, end: number, headTime: number) {
  const ctx = fit(canvas);
  const w = canvas.clientWidth, h = canvas.clientHeight;
  if (!model) return;
  const x = (t: number) => (t / end) * w;
  const rowH = Math.round(h * SCENE_ROW_SHARE);
  ctx.font = "11px ui-monospace, monospace";
  ctx.textBaseline = "middle";

  for (const s of model.scenes) {
    const x0 = x(s.t0), x1 = x(s.t1);
    ctx.fillStyle = sceneColor(s.scene);
    ctx.fillRect(x0, 0, Math.max(1, x1 - x0), rowH);
    ctx.fillStyle = SCENE_EDGE_COLOR;
    ctx.fillRect(x0, 0, 1, rowH);
    spanLabel(ctx, s.label, x0, x1, rowH / 2, SCENE_TEXT_COLOR);
  }

  ctx.font = "9px ui-monospace, monospace";
  for (const p of model.plays) {
    const x0 = x(p.t0), x1 = x(p.t1);
    ctx.fillStyle = p.kind === "action" ? ACTION_COLOR : TRANSITION_COLOR;
    ctx.fillRect(x0, rowH + 1, Math.max(2, x1 - x0), h - rowH - 1);
    spanLabel(ctx, p.label, x0, x1, rowH + (h - rowH) / 2, PLAY_TEXT_COLOR);
  }

  ctx.fillStyle = MARK_COLOR;
  for (const m of model.marks) {
    const mx = x(m.t);
    ctx.beginPath();
    ctx.moveTo(mx - MARK_PX, 0);
    ctx.lineTo(mx + MARK_PX, 0);
    ctx.lineTo(mx, MARK_PX + 1);
    ctx.fill();
  }

  drawHead(ctx, x(headTime), h);
}

/** What the scenes strip shows at time t, for its tooltip: the play over the scene first */
export function scenesTip(model: SceneModel | null, t: number, tolerance: number): string {
  if (!model) return "";
  const mark = model.marks.find((m) => Math.abs(m.t - t) <= tolerance);
  const now = model.at(t);
  const parts = [mark?.title, now.playing?.title, now.scene?.title].filter(Boolean);
  return parts.join("\n");
}

export function drawTimeline(
  canvas: HTMLCanvasElement,
  rec: Recording | null,
  headTime: number,
  loop: [number, number] | null,
) {
  const ctx = fit(canvas);
  const w = canvas.clientWidth, h = canvas.clientHeight;
  if (!rec || !rec.length) return;
  const end = rec.end;
  const x = (t: number) => (t / end) * w;
  const frameEnd = (i: number) => (i + 1 < rec.length ? rec.times[i + 1] : end);

  if (loop) {
    ctx.fillStyle = LOOP_COLOR;
    ctx.fillRect(x(rec.times[loop[0]]), 0, x(frameEnd(loop[1])) - x(rec.times[loop[0]]), h);
  }

  ctx.fillStyle = EFFECT_COLOR;
  for (let i = 0; i < rec.length; ) {
    if (!rec.inEffect(i)) {
      i++;
      continue;
    }
    const start = i;
    while (i < rec.length && rec.inEffect(i)) i++;
    ctx.fillRect(x(rec.times[start]), 0, Math.max(1, x(rec.times[i - 1]) - x(rec.times[start])), h);
  }

  const step = SECOND_STEPS.find((s) => x(s * MS_PER_S) >= LABEL_GAP_PX) ?? SECOND_STEPS[SECOND_STEPS.length - 1];
  ctx.font = "10px ui-monospace, monospace";
  for (let s = 0; s * MS_PER_S <= end; s += step) {
    const tx = x(s * MS_PER_S);
    ctx.fillStyle = TICK_COLOR;
    ctx.fillRect(tx, h - 6, 1, 6);
    ctx.fillStyle = LABEL_COLOR;
    ctx.fillText(`${s}s`, tx + 3, h - 2);
  }

  for (const l of rec.logs) {
    ctx.fillStyle = l.level >= LOG_ERROR ? PANIC_COLOR : l.level === LOG_WARN ? WARN_COLOR : LOG_TICK_COLOR;
    ctx.fillRect(x(l.time), 0, 1, 5);
  }

  if (rec.panic) {
    ctx.fillStyle = PANIC_COLOR;
    ctx.fillRect(x(rec.times[Math.min(rec.panic.frame, rec.length - 1)]) - 1, 0, 3, h);
  }

  drawHead(ctx, x(headTime), h);
}

export function drawPixelHistory(
  canvas: HTMLCanvasElement,
  rec: Recording | null,
  pin: { x: number; y: number } | null,
  headTime: number,
) {
  const ctx = fit(canvas);
  const w = canvas.clientWidth, h = canvas.clientHeight;
  if (!rec || !pin || !rec.length) return;
  const end = rec.end;
  const x = (t: number) => (t / end) * w;

  const channels = rec.gray ? 1 : 3;
  const step = Math.max(1, Math.floor(rec.length / (w * 2)));
  for (let c = 0; c < channels; c++) {
    ctx.strokeStyle = rec.gray ? GRAY_COLOR : CHANNEL_COLORS[c];
    ctx.lineWidth = 1.25;
    ctx.beginPath();
    let py = 0;
    for (let i = 0; i < rec.length; i += step) {
      const px = x(rec.times[i]);
      /* A held value is a step: flat until the next frame */
      if (i > 0) ctx.lineTo(px, py);
      py = h - 2 - (rec.pixel(i, pin.x, pin.y)[c] / 255) * (h - 4);
      if (i === 0) ctx.moveTo(px, py);
      else ctx.lineTo(px, py);
    }
    ctx.lineTo(w, py);
    ctx.stroke();
  }

  drawHead(ctx, x(headTime), h);
}

