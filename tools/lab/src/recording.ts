import { decode, type Rgb } from "./color";
import { FRAME_FLAG_EFFECT, type LogLine, type TraceEvent } from "./protocol";

/** The frames of one run, as pushed: a view per frame into the batches they came in */
export class Recording {
  readonly frames: Uint32Array[] = [];
  readonly times: number[] = [];
  readonly flags: number[] = [];
  readonly logs: LogLine[] = [];
  readonly trace: TraceEvent[] = [];
  panic: { file: string; line: number; frame: number } | null = null;
  done = false;
  /** How far a live session has run: past its last frame while a scene holds still */
  liveTime = 0;

  constructor(
    readonly width: number,
    readonly height: number,
    readonly gray: boolean,
  ) {}

  get length() {
    return this.frames.length;
  }

  get duration() {
    return this.times.length ? this.times[this.times.length - 1] : 0;
  }

  /** The run's span in time, what the strips are drawn over */
  get end() {
    return Math.max(this.duration, this.liveTime, 1);
  }

  add(pixels: Uint32Array, times: Uint32Array, flags: Uint8Array) {
    const size = this.width * this.height;
    for (let i = 0; i < times.length; i++) {
      this.frames.push(pixels.subarray(i * size, (i + 1) * size));
      this.times.push(times[i]);
      this.flags.push(flags[i]);
    }
  }

  inEffect(idx: number) {
    return (this.flags[idx] & FRAME_FLAG_EFFECT) !== 0;
  }

  /** Last frame shown at time t: the one pushed at or before it */
  frameAt(t: number): number {
    let lo = 0, hi = this.times.length - 1;
    if (hi < 0 || t < this.times[0]) return 0;
    while (lo < hi) {
      const mid = (lo + hi + 1) >> 1;
      if (this.times[mid] <= t) lo = mid;
      else hi = mid - 1;
    }
    return lo;
  }

  pixel(idx: number, x: number, y: number): Rgb {
    return decode(this.frames[idx][y * this.width + x], this.gray);
  }

  raw(idx: number, x: number, y: number): number {
    return this.frames[idx][y * this.width + x];
  }
}
