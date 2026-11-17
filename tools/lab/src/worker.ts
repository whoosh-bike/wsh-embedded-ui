/// <reference lib="webworker" />
import { LOG_PRINT, type FromWorker, type LogLine, type ToWorker, type TraceEvent, type Variant } from "./protocol";

/**
 * Runs the wasm. Every run gets a fresh module instance, so the C starts from the same
 * state each time and a run replays bit for bit. Frames leave in batches as they are
 * pushed: a run computes in milliseconds, the batches only keep a big panel's memory flat.
 * A live target stays open between messages: the UI advances its time and presses inputs.
 * The scene manager's steps come out as they happen, a transition or an action named by
 * its C function - looked up in the module's name section by its table index.
 */

declare const __LAB_TARGET__: string;

interface LabModule {
  HEAPU32: Uint32Array;
  wasmTable: WebAssembly.Table;
  UTF8ToString(ptr: number): string;
  _Lab_Start(w: number, h: number): number;
  _Lab_ActionsNum(): number;
  _Lab_ActionName(idx: number): number;
  _Lab_RunAction(idx: number): void;
  _Lab_IsLive(): number;
  _Lab_InputsNum(): number;
  _Lab_InputName(idx: number): number;
  _Lab_InputGroup(idx: number): number;
  _Lab_Input(idx: number): void;
  _Lab_ScenesNum(): number;
  _Lab_SceneName(idx: number): number;
  _Lab_Advance(untilMs: number): void;
  _Lab_SetBright(scale: number): void;
  _Lab_TimeMs(): number;
  _Lab_MaxWidth(): number;
  _Lab_MaxHeight(): number;
  _Lab_IsGray(): number;
}

interface Hooks {
  onFrame(ptr: number, idx: number, timeMs: number, flags: number): void;
  onPanic(file: string, line: number): void;
  onLog(level: number, text: string, timeMs: number): void;
  onTrace(kind: number, cause: number, scene: number, from: number, fn: number, input: string, timeMs: number): void;
  print(text: string): void;
  printErr(text: string): void;
}

type Factory = (hooks: Hooks) => Promise<LabModule>;
type Start = Extract<ToWorker, { type: "run" | "live" }>;

const BATCH_FRAMES = 64;
const NAME_SECTION_FUNCTIONS = 1;
const factories = new Map<Variant, Promise<Factory>>();
const fnNames = new Map<Variant, Promise<Map<number, string>>>();

const post = (msg: FromWorker) => postMessage(msg);

const wasmUrl = (variant: Variant, ext: string) => `/wasm/${__LAB_TARGET__}-${variant}.${ext}?t=${Date.now()}`;

function factory(variant: Variant): Promise<Factory> {
  let f = factories.get(variant);
  if (!f) {
    f = import(/* @vite-ignore */ wasmUrl(variant, "mjs")).then((m) => m.default as Factory);
    factories.set(variant, f);
  }
  return f;
}

/** Function index to C name, from the "name" custom section --profiling-funcs keeps */
function parseNames(section: ArrayBuffer): Map<number, string> {
  const b = new Uint8Array(section);
  const names = new Map<number, string>();
  const text = new TextDecoder();
  let p = 0;
  const leb = () => {
    let r = 0, shift = 0, byte;
    do {
      byte = b[p++];
      r |= (byte & 0x7f) << shift;
      shift += 7;
    } while (byte & 0x80);
    return r >>> 0;
  };
  while (p < b.length) {
    const id = b[p++], end = leb() + p;
    if (id === NAME_SECTION_FUNCTIONS) {
      for (let n = leb(); n > 0; n--) {
        const idx = leb(), len = leb();
        names.set(idx, text.decode(b.subarray(p, p + len)));
        p += len;
      }
    }
    p = end;
  }
  return names;
}

function names(variant: Variant): Promise<Map<number, string>> {
  let n = fnNames.get(variant);
  if (!n) {
    n = fetch(wasmUrl(variant, "wasm"))
      .then((r) => r.arrayBuffer())
      .then((bytes) => WebAssembly.compile(bytes))
      .then((mod) => {
        const section = WebAssembly.Module.customSections(mod, "name")[0];
        return section ? parseNames(section) : new Map<number, string>();
      });
    fnNames.set(variant, n);
  }
  return n;
}

/** "GfxUi_Transition_ScreenOff" -> "screen_off", "GfxUi_Popup_Zone" -> "popup_zone":
 * the app's prefix off, as SCENARIO-style docs name them */
function shortName(full: string): string {
  const cut = full.lastIndexOf("Transition_");
  const rest = cut >= 0 ? full.slice(cut + "Transition_".length) : full.slice(full.indexOf("_") + 1) || full;
  return rest.replace(/([a-z0-9])([A-Z])/g, "$1_$2").toLowerCase();
}

const noHooks: Hooks = { onFrame() {}, onPanic() {}, onLog() {}, onTrace() {}, print() {}, printErr() {} };

const strings = (n: number, name: (i: number) => number, m: LabModule) =>
  Array.from({ length: n }, (_, i) => m.UTF8ToString(name(i)));

async function info(variant: Variant) {
  const m = await (await factory(variant))(noHooks);
  m._Lab_Start(Math.min(8, m._Lab_MaxWidth()), Math.min(8, m._Lab_MaxHeight()));
  post({
    type: "info",
    actions: strings(m._Lab_ActionsNum(), m._Lab_ActionName, m),
    inputs: strings(m._Lab_InputsNum(), m._Lab_InputName, m),
    inputGroups: strings(m._Lab_InputsNum(), m._Lab_InputGroup, m),
    scenes: strings(m._Lab_ScenesNum(), m._Lab_SceneName, m),
    live: !!m._Lab_IsLive(),
    maxWidth: m._Lab_MaxWidth(),
    maxHeight: m._Lab_MaxHeight(),
    gray: !!m._Lab_IsGray(),
  });
}

/** One module instance and the frames and lines on their way to the UI */
class Session {
  mod!: LabModule;
  private readonly size: number;
  private readonly pixels: Uint32Array;
  private readonly times = new Uint32Array(BATCH_FRAMES);
  private readonly flags = new Uint8Array(BATCH_FRAMES);
  private count = 0;
  private pushed = 0;
  private lines: LogLine[] = [];
  private traces: TraceEvent[] = [];
  private fnNames = new Map<number, string>();

  constructor(readonly req: Start) {
    this.size = req.width * req.height;
    this.pixels = new Uint32Array(this.size * BATCH_FRAMES);
  }

  /** @return false when the C refuses the panel */
  async open(): Promise<boolean> {
    const line = (level: number, text: string, time: number) =>
      this.lines.push({ level, text, frame: this.pushed, time });
    const hooks: Hooks = {
      onFrame: (ptr, _idx, timeMs, frameFlags) => {
        this.pixels.set(this.mod.HEAPU32.subarray(ptr >> 2, (ptr >> 2) + this.size), this.count * this.size);
        this.times[this.count] = timeMs;
        this.flags[this.count] = frameFlags;
        this.pushed++;
        if (++this.count === BATCH_FRAMES) this.flush();
      },
      onLog: (level, text, timeMs) => line(level, text, timeMs),
      onTrace: (kind, cause, scene, from, fn, input, timeMs) => {
        const full = this.fnName(fn);
        /* A u32 crosses into JS signed: SELF would read as -1 */
        this.traces.push({
          kind,
          cause,
          scene: scene >>> 0,
          from: from >>> 0,
          fn: full && shortName(full),
          fnFull: full,
          input,
          frame: this.pushed,
          time: timeMs,
        });
      },
      print: (text) => line(LOG_PRINT, text, this.mod?._Lab_TimeMs() ?? 0),
      printErr: (text) => line(LOG_PRINT, text, this.mod?._Lab_TimeMs() ?? 0),
      onPanic: (file, l) => post({ type: "panic", run: this.req.run, file, line: l, frame: this.pushed }),
    };
    this.fnNames = await names(this.req.variant);
    this.mod = await (await factory(this.req.variant))(hooks);
    if (!this.mod._Lab_Start(this.req.width, this.req.height)) return false;
    this.mod._Lab_SetBright(this.req.bright);
    return true;
  }

  /** A function pointer is an index into the table; the function there knows its own index */
  private fnName(ptr: number): string {
    if (!ptr) return "";
    const fn = this.mod.wasmTable.get(ptr) as { name?: string } | null;
    return (fn?.name && this.fnNames.get(Number(fn.name))) || `fn#${ptr}`;
  }

  flush() {
    if (this.traces.length) {
      post({ type: "traces", run: this.req.run, events: this.traces });
      this.traces = [];
    }
    if (this.lines.length) {
      post({ type: "logs", run: this.req.run, lines: this.lines });
      this.lines = [];
    }
    if (!this.count) return;
    const n = this.count;
    post({
      type: "frames",
      run: this.req.run,
      pixels: this.pixels.slice(0, n * this.size),
      times: this.times.slice(0, n),
      flags: this.flags.slice(0, n),
    });
    this.count = 0;
  }

  advance(untilMs: number) {
    try {
      this.mod._Lab_Advance(untilMs);
    } finally {
      this.flush();
    }
    post({ type: "advanced", run: this.req.run, time: this.mod._Lab_TimeMs() });
  }
}

let live: Session | null = null;

async function start(req: Start) {
  live = null;
  const s = new Session(req);
  if (!(await s.open())) {
    post({ type: "done", run: req.run, ok: false });
    return;
  }

  if (req.type === "run") {
    try {
      s.mod._Lab_RunAction(req.action);
    } finally {
      s.flush();
    }
    post({ type: "done", run: req.run, ok: true });
    return;
  }

  /* A live session replays the inputs it had, each at its time, so a restart lands
   * on the same picture */
  live = s;
  try {
    if (req.start >= 0) s.mod._Lab_RunAction(req.start);
  } finally {
    s.flush();
  }
  for (const input of req.inputs) {
    s.mod._Lab_Advance(input.time);
    s.mod._Lab_Input(input.idx);
  }
  s.advance(req.until);
}

onmessage = async (e: MessageEvent<ToWorker>) => {
  const msg = e.data;
  try {
    switch (msg.type) {
      case "info":
        await info(msg.variant);
        break;
      case "run":
      case "live":
        await start(msg);
        break;
      case "advance":
        if (live?.req.run === msg.run) live.advance(msg.until);
        break;
      case "input":
        if (live?.req.run !== msg.run) break;
        live.mod._Lab_Input(msg.idx);
        post({ type: "inputAt", run: msg.run, idx: msg.idx, time: live.mod._Lab_TimeMs() });
        live.flush();
        break;
    }
  } catch (err) {
    post({ type: "failure", message: err instanceof Error ? `${err.message}\n${err.stack ?? ""}` : String(err) });
  }
};
