/** Messages between the UI and the worker that runs the wasm */

export const VARIANTS = ["COLOR-RGB", "GRAY-GRAY"] as const;
export type Variant = (typeof VARIANTS)[number];

export const FRAME_FLAG_EFFECT = 1 << 0;

export interface RunRequest {
  type: "run";
  run: number;
  variant: Variant;
  width: number;
  height: number;
  action: number;
  bright: number;
}

/** An input pressed in a live session, kept so a restart replays it */
export interface InputEvent {
  idx: number;
  time: number;
}

/** Starts a live target: runs action `start` (-1 for none), replays `inputs`, then runs to `until` ms */
export interface LiveRequest {
  type: "live";
  run: number;
  variant: Variant;
  width: number;
  height: number;
  bright: number;
  start: number;
  inputs: InputEvent[];
  until: number;
}

export interface AdvanceRequest {
  type: "advance";
  run: number;
  until: number;
}

export interface InputRequest {
  type: "input";
  run: number;
  idx: number;
}

export interface InfoRequest {
  type: "info";
  variant: Variant;
}

export type ToWorker = RunRequest | LiveRequest | AdvanceRequest | InputRequest | InfoRequest;

export interface Info {
  type: "info";
  actions: string[];
  inputs: string[];
  /** The heading each input comes under, "" for none */
  inputGroups: string[];
  /** The target's scene names, by index: empty when it names none */
  scenes: string[];
  live: boolean;
  maxWidth: number;
  maxHeight: number;
  gray: boolean;
}

/** Frames of one batch back to back, width * height pixels each, 0x00BBGGRR */
export interface Frames {
  type: "frames";
  run: number;
  pixels: Uint32Array;
  times: Uint32Array;
  flags: Uint8Array;
}

/** LOG_LVL_t of host/sim/log.h; a plain printf arrives as PRINT, the scene manager's steps as SCENE */
export const LOG_LEVELS = ["TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITIC"] as const;
export const LOG_PRINT = -1;
export const LOG_SCENE = -2;

/** WeUi_Gfx_SceneMgr_TraceKind_t and _Cause_t of weui_gfx_scenemgr.h */
export const TRACE = { ROUTED: 0, SWITCH: 1, PLAY: 2, SHOWN: 3, ACTION: 4, ACTION_DONE: 5 } as const;
export const CAUSES = ["start", "input", "timeout", "request", "clear", "back"] as const;
export const CAUSE = { HOME: 0, INPUT: 1, TIMEOUT: 2, REQUEST: 3, CLEAR: 4, HANDBACK: 5 } as const;
/** WEUI_GFX_SCENEMGR_SELF: no scene, or "stay" for a ROUTED */
export const SCENE_NONE = 0xffffffff;

/** One step of the scene manager, as WeUi_Gfx_SceneMgr_Trace_t */
export interface TraceEvent {
  kind: number;
  cause: number;
  scene: number;
  from: number;
  /** The transition or the action, short - "screen_off" - and as named in C */
  fn: string;
  fnFull: string;
  /** A ROUTED input, as the target names it */
  input: string;
  /** The next frame to be pushed, as for a log line */
  frame: number;
  time: number;
}

export interface Traces {
  type: "traces";
  run: number;
  events: TraceEvent[];
}

export interface LogLine {
  level: number;
  text: string;
  /** The frame being drawn when it was printed - the next one pushed */
  frame: number;
  time: number;
}

export interface Logs {
  type: "logs";
  run: number;
  lines: LogLine[];
}

export interface Done {
  type: "done";
  run: number;
  ok: boolean;
}

/** A live session ran up to `time` ms */
export interface Advanced {
  type: "advanced";
  run: number;
  time: number;
}

export interface InputAt {
  type: "inputAt";
  run: number;
  idx: number;
  time: number;
}

export interface Panic {
  type: "panic";
  run: number;
  file: string;
  line: number;
  frame: number;
}

export interface Failure {
  type: "failure";
  message: string;
}

export type FromWorker = Info | Frames | Logs | Traces | Done | Advanced | InputAt | Panic | Failure;
