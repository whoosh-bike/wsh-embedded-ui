import { CAUSE, CAUSES, SCENE_NONE, TRACE, type TraceEvent } from "./protocol";

/**
 * The scene manager's steps folded into what a person asks of a run: which scene was up
 * when, what played over it - a transition, an action that borrowed the panel - and every
 * move from one scene to another, by what and with what. All of it in virtual time.
 */

export interface Span {
  kind: "scene" | "transition" | "action";
  t0: number;
  t1: number;
  label: string;
  title: string;
  scene: number;
}

/** One way from a scene to another: why, with what, and every time it was taken */
export interface Move {
  from: number;
  to: number;
  why: string;
  transition: string;
  times: number[];
}

export interface Mark {
  t: number;
  title: string;
}

export interface SceneNow {
  scene: Span | null;
  arrival: { from: number; why: string; transition: string } | null;
  playing: Span | null;
  lastMove: Move | null;
}

/** "wiggle in off: → standby" - what an input or a timeout was answered with */
export function routedText(e: TraceEvent, name: (scene: number) => string): string {
  const what = e.cause === CAUSE.TIMEOUT ? "timeout" : e.input || "input";
  const to = e.scene === SCENE_NONE ? "stays" : e.scene === e.from ? "enters again" : `→ ${name(e.scene)}`;
  return `${what} in ${name(e.from)}: ${to}`;
}

/** A step as a console line; PLAY of a switch says nothing SWITCH did not */
export function traceLine(e: TraceEvent, name: (scene: number) => string): string | null {
  switch (e.kind) {
    case TRACE.ROUTED:
      return routedText(e, name);
    case TRACE.SWITCH: {
      /* An input's switch follows the line naming the input */
      const why = e.cause === CAUSE.INPUT ? "" : ` · ${CAUSES[e.cause] ?? "?"}`;
      return `${name(e.from)} → ${name(e.scene)}${why}${e.fn ? ` / ${e.fn}` : ", cut"}`;
    }
    case TRACE.PLAY:
      return e.cause === CAUSE.HANDBACK ? `back into ${name(e.scene)} / ${e.fn}` : null;
    case TRACE.SHOWN:
      return `${name(e.scene)} on the panel, its time runs`;
    case TRACE.ACTION:
      return `action ${e.fn} over ${name(e.scene)}`;
    case TRACE.ACTION_DONE:
      return `action ${e.fn} done`;
    default:
      return null;
  }
}

export class SceneModel {
  readonly scenes: Span[] = [];
  readonly plays: Span[] = [];
  readonly marks: Mark[] = [];
  readonly moves: Move[] = [];
  private readonly arrivals: SceneNow["arrival"][] = [];
  /** Every switch, in time order */
  private readonly taken: { t0: number; move: Move }[] = [];

  constructor(
    trace: TraceEvent[],
    end: number,
    readonly names: string[],
  ) {
    const open: Partial<Record<"transition" | "action", Span>> = {};
    let routed: TraceEvent | null = null;

    for (const e of trace) {
      switch (e.kind) {
        case TRACE.ROUTED:
          routed = e;
          this.marks.push({ t: e.time, title: routedText(e, (i) => this.name(i)) });
          break;

        case TRACE.SWITCH: {
          const last = this.scenes[this.scenes.length - 1];
          if (last) last.t1 = e.time;
          const why = this.why(e, routed);
          this.scenes.push({ kind: "scene", t0: e.time, t1: end, label: this.name(e.scene), title: "", scene: e.scene });
          this.arrivals.push({ from: e.from, why, transition: e.fn });
          if (e.from !== SCENE_NONE) this.take(e, why);
          routed = null;
          break;
        }

        case TRACE.PLAY:
          open.transition = {
            kind: "transition",
            t0: e.time,
            t1: end,
            label: e.fn,
            title: `${e.fn} into ${this.name(e.scene)}${e.cause === CAUSE.HANDBACK ? ", handing the panel back" : ""} · ${e.fnFull}`,
            scene: e.scene,
          };
          this.plays.push(open.transition);
          break;

        case TRACE.SHOWN:
          if (open.transition) open.transition.t1 = e.time;
          open.transition = undefined;
          break;

        case TRACE.ACTION:
          open.action = { kind: "action", t0: e.time, t1: end, label: e.fn, title: `action ${e.fn} · ${e.fnFull}`, scene: e.scene };
          this.plays.push(open.action);
          break;

        case TRACE.ACTION_DONE:
          if (open.action) open.action.t1 = e.time;
          open.action = undefined;
          break;
      }
    }

    this.scenes.forEach((s, i) => (s.title = this.sceneTitle(s, this.arrivals[i])));
    /* An action drawn first, so a transition it plays shows on top of it */
    this.plays.sort((a, b) => (a.kind === b.kind ? a.t0 - b.t0 : a.kind === "action" ? -1 : 1));
  }

  name(scene: number): string {
    if (scene === SCENE_NONE) return "-";
    return this.names[scene] || `#${scene}`;
  }

  /** What the scene was, and what played, at time t */
  at(t: number): SceneNow {
    const i = this.indexAt(this.scenes, t);
    const lastTaken = this.indexAt(this.taken, t);
    return {
      scene: i >= 0 ? this.scenes[i] : null,
      arrival: i >= 0 ? this.arrivals[i] : null,
      playing: this.playingAt(t),
      lastMove: lastTaken >= 0 ? this.taken[lastTaken].move : null,
    };
  }

  playingAt(t: number): Span | null {
    let found: Span | null = null;
    for (const p of this.plays) if (p.t0 <= t && t < p.t1) found = p;
    return found;
  }

  /** The first time a move was taken after t, round to its first */
  nextTime(move: Move, t: number): number {
    return move.times.find((time) => time > t) ?? move.times[0];
  }

  whyText(a: { why: string; transition: string }): string {
    return a.transition ? `${a.why} / ${a.transition}` : `${a.why}, cut`;
  }

  private take(e: TraceEvent, why: string) {
    let move = this.moves.find((m) => m.from === e.from && m.to === e.scene && m.why === why && m.transition === e.fn);
    if (!move) {
      move = { from: e.from, to: e.scene, why, transition: e.fn, times: [] };
      this.moves.push(move);
    }
    move.times.push(e.time);
    this.taken.push({ t0: e.time, move });
  }

  private why(e: TraceEvent, routed: TraceEvent | null): string {
    if (e.cause === CAUSE.INPUT) return routed?.input || "input";
    return CAUSES[e.cause] ?? "?";
  }

  private sceneTitle(s: Span, arrival: SceneNow["arrival"]): string {
    const len = `${((s.t1 - s.t0) / 1000).toFixed(2)} s`;
    if (!arrival || arrival.from === SCENE_NONE) return `${s.label} · ${len} · ${arrival?.why ?? ""}`;
    return `${s.label} · ${len} · from ${this.name(arrival.from)}: ${this.whyText(arrival)}`;
  }

  private indexAt(spans: { t0: number }[], t: number): number {
    let lo = 0, hi = spans.length - 1, found = -1;
    while (lo <= hi) {
      const mid = (lo + hi) >> 1;
      if (spans[mid].t0 <= t) (found = mid), (lo = mid + 1);
      else hi = mid - 1;
    }
    return found;
  }
}
