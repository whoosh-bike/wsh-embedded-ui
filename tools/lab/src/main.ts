import { drawPixelHistory, drawScenes, drawTimeline, scenesTip, timeFromClick } from "./charts";
import { decode, hex, luma, toHsb } from "./color";
import { LogConsole, levelName } from "./console";
import { SceneGraph } from "./graph";
import { initLayout } from "./layout";
import { Panel, type Look } from "./panel";
import {
  LOG_LEVELS,
  LOG_PRINT,
  LOG_SCENE,
  SCENE_NONE,
  VARIANTS,
  type FromWorker,
  type Info,
  type InputEvent,
  type LogLine,
  type Variant,
} from "./protocol";
import { Recording } from "./recording";
import { SceneModel, traceLine } from "./scenes";

declare const __LAB_TARGET__: string;

/**
 * The UI. A run is the target's action played from a fresh start; its frames land in a
 * Recording, and everything here only looks at that recording. Playback walks it on the
 * frames' own virtual timestamps, so a paused or scrubbed transition is the real one.
 * The view state survives a reload: a rebuild after a saved .c reopens the same run at
 * the same frame. A live session starts as the target does, or in one of its actions -
 * a scene - and is the same session for as long as nothing restarts it.
 */

const SIZES = [
  [32, 8],
  [90, 25],
  [128, 64],
  [416, 240],
] as const;
const SPEEDS = [0.1, 0.25, 0.5, 1, 2, 4];
const ZOOMS = [0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32]; /* 0 fits the stage */
const FIT_ZOOM_MAX = 32;
const STEP_BIG = 10;
const STATE_KEY = `weui-lab:${__LAB_TARGET__}`;
/* A live target is run this far ahead of the playhead, so playback never waits on it */
const LIVE_LOOKAHEAD_MS = 250;
const INPUT_KEYS = "123456789";
/* Within this many px of a mark, the scenes strip's tooltip names it */
const MARK_HOVER_PX = 4;

interface State {
  variant: Variant;
  width: number;
  height: number;
  action: number;
  bright: number;
  look: Look;
  grid: boolean;
  diff: boolean;
  zoom: number;
  speed: number;
  frame: number;
  pin: { x: number; y: number } | null;
  /** A live session: the action it started with (-1 as the target starts), what was
   * pressed and when, and how far it ran */
  start: number;
  inputs: InputEvent[];
  liveUntil: number;
}

const $ = <T extends HTMLElement>(id: string) => document.getElementById(id) as T;

const state: State = {
  variant: "COLOR-RGB",
  width: 90,
  height: 25,
  action: 0,
  bright: 1,
  look: "led",
  grid: false,
  diff: false,
  zoom: 0,
  speed: 1,
  frame: 0,
  pin: null,
  start: -1,
  inputs: [],
  liveUntil: 0,
  ...loadState(),
};

function loadState(): Partial<State> {
  try {
    return JSON.parse(sessionStorage.getItem(STATE_KEY) ?? "{}");
  } catch {
    return {};
  }
}

function saveState() {
  try {
    sessionStorage.setItem(STATE_KEY, JSON.stringify(state));
  } catch {
    /* a private window keeps no state: the lab still works, it only forgets */
  }
}

const panel = new Panel($<HTMLCanvasElement>("panel"));
const logs = new LogConsole($("console"), (frame) => (play(false), seek(frame)));
const graph = new SceneGraph(document.getElementById("graph") as unknown as SVGSVGElement, $("moves"), (move) => {
  if (!model) return;
  play(false);
  seekTime(model.nextTime(move, playTime));
});
const worker = new Worker(new URL("./worker.ts", import.meta.url), { type: "module" });

let info: Info | null = null;
let rec: Recording | null = null;
let runId = 0;
let head = 0;
let playing = false;
let playTime = 0;
let lastTick = 0;
let ticking = false;
let loop: [number, number] | null = null;
let hover: { x: number; y: number } | null = null;
let seekOnDone: number | null = null;
let simTime = 0;
let advancing = false;
let liveReady = false;
/** Built from rec.trace when asked for, dropped when more comes in */
let model: SceneModel | null = null;
/** The input each key 1-9 presses */
let keyInputs: number[] = [];

// =================== Worker ===================

worker.onmessage = (e: MessageEvent<FromWorker>) => {
  const msg = e.data;
  switch (msg.type) {
    case "info":
      info = msg;
      buildActions();
      buildSizes();
      startRun(state.action, state.frame || null);
      break;
    case "frames":
      if (msg.run !== runId || !rec) return;
      rec.add(msg.pixels, msg.times, msg.flags);
      render();
      break;
    case "logs":
      if (msg.run !== runId || !rec) return;
      addLogLines(msg.lines);
      break;
    case "traces": {
      if (msg.run !== runId || !rec) return;
      rec.trace.push(...msg.events);
      model = null;
      const name = (scene: number) => sceneModel()!.name(scene);
      addLogLines(
        msg.events.flatMap((e) => {
          const text = traceLine(e, name);
          return text ? [{ level: LOG_SCENE, text, frame: e.frame, time: e.time }] : [];
        }),
      );
      break;
    }
    case "panic":
      if (msg.run !== runId || !rec) return;
      rec.panic ??= { file: msg.file, line: msg.line, frame: msg.frame };
      showPanic();
      break;
    case "done":
      if (msg.run !== runId || !rec) return;
      rec.done = true;
      if (!msg.ok) {
        status("panel refused: larger than the build's maximum");
        break;
      }
      status(`${rec.length} frames, ${(rec.duration / 1000).toFixed(2)} s`);
      settle();
      break;
    case "advanced":
      if (msg.run !== runId || !rec) return;
      simTime = msg.time;
      rec.liveTime = simTime;
      /* The last scene runs up to the session's end, which just moved */
      model = null;
      advancing = false;
      state.liveUntil = simTime;
      saveState();
      status(`live · ${(simTime / 1000).toFixed(1)} s · ${rec.length} frames · ${state.inputs.length} inputs`);
      if (!liveReady) {
        liveReady = true;
        settle();
      }
      break;
    case "inputAt":
      if (msg.run !== runId) return;
      state.inputs.push({ idx: msg.idx, time: msg.time });
      saveState();
      break;
    case "failure":
      status("worker failed - see the console");
      console.error(msg.message);
      showPanic(msg.message);
      break;
  }
};

/** A run is in, or a live session replayed: land where the user was, or play */
function settle() {
  if (!rec) return;
  if (seekOnDone !== null) {
    seek(Math.min(seekOnDone, rec.length - 1));
    seekOnDone = null;
  } else if (rec.panic) {
    seek(rec.panic.frame);
  } else {
    play(true);
  }
}

/** Lines from the C and from the trace come in separate batches: kept in time order */
function addLogLines(lines: LogLine[]) {
  if (!rec || !lines.length) return;
  const last = rec.logs[rec.logs.length - 1];
  rec.logs.push(...lines);
  if (last && lines[0].time < last.time) rec.logs.sort((a, b) => a.time - b.time);
  logs.set(rec.logs);
  showLogCount();
}

function sceneModel(): SceneModel | null {
  if (!rec || (!rec.trace.length && !info?.scenes.length)) return null;
  model ??= new SceneModel(rec.trace, rec.end, info?.scenes ?? []);
  return model;
}

function requestInfo() {
  status("loading wasm…");
  worker.postMessage({ type: "info", variant: state.variant });
}

function startRun(action: number, seekTo: number | null = null) {
  if (!info) return;
  state.action = action;
  runId++;
  rec = new Recording(state.width, state.height, info.gray);
  model = null;
  head = 0;
  playTime = 0;
  loop = null;
  seekOnDone = seekTo;
  play(false);
  showPanic();
  markActive();
  logs.set(rec.logs);
  showLogCount();
  status("running…");
  const panelReq = { run: runId, variant: state.variant, width: state.width, height: state.height, bright: state.bright };
  if (info.live) {
    simTime = 0;
    advancing = true;
    liveReady = false;
    if (state.start >= info.actions.length) state.start = -1;
    worker.postMessage({ type: "live", ...panelReq, start: state.start, inputs: state.inputs, until: state.liveUntil });
  } else {
    worker.postMessage({ type: "run", ...panelReq, action });
  }
  saveState();
}

/** A live target starts over from nothing pressed - in `start`, or where it was */
function restartLive(start = state.start) {
  state.start = start;
  state.inputs = [];
  state.liveUntil = 0;
  state.frame = 0;
  startRun(state.action);
}

function press(idx: number) {
  if (!info?.live || !rec || idx >= info.inputs.length) return;
  worker.postMessage({ type: "input", run: runId, idx });
  /* The input lands where the session is, so the playhead goes there to show it */
  loop = null;
  playTime = Math.max(playTime, simTime);
  play(true);
}

// =================== Playback ===================

function seek(idx: number) {
  if (!rec || !rec.length) return;
  head = Math.max(0, Math.min(rec.length - 1, idx));
  playTime = rec.times[head];
  state.frame = head;
  saveState();
  render();
}

/** To a moment rather than a frame: within a still scene the playhead moves, the frame not */
function seekTime(t: number) {
  if (!rec || !rec.length) return;
  playTime = Math.max(0, Math.min(rec.end, t));
  head = rec.frameAt(playTime);
  state.frame = head;
  saveState();
  render();
}

function play(on: boolean) {
  playing = on;
  $("play").textContent = on ? "⏸" : "▶︎";
  lastTick = performance.now();
  if (on && rec && head >= rec.length - 1 && rec.done) seek(loop ? loop[0] : 0);
  if (on && !ticking) {
    ticking = true;
    requestAnimationFrame(tick);
  }
}

function tick(now: number) {
  if (!playing || !rec) {
    ticking = false;
    return;
  }
  playTime += (now - lastTick) * state.speed;
  lastTick = now;

  if (info?.live && !loop) {
    if (!advancing && playTime + LIVE_LOOKAHEAD_MS > simTime) {
      advancing = true;
      worker.postMessage({ type: "advance", run: runId, until: Math.ceil(playTime + LIVE_LOOKAHEAD_MS) });
    }
    playTime = Math.min(playTime, simTime);
  }

  const end = loop ? rec.times[loop[1]] : rec.duration;
  if (playTime > end && (rec.done || loop)) {
    if (loop) playTime = rec.times[loop[0]];
    else {
      seek(rec.length - 1);
      play(false);
      ticking = false;
      return;
    }
  }

  const idx = rec.frameAt(playTime);
  if (idx !== head) {
    head = idx;
    state.frame = idx;
    render();
  } else {
    renderStrips();
  }
  requestAnimationFrame(tick);
}

// =================== Drawing ===================

function fitZoom(): number {
  if (state.zoom) return state.zoom;
  const stage = $("stage");
  const z = Math.floor(Math.min(stage.clientWidth / state.width, stage.clientHeight / state.height) * 0.94);
  return Math.max(1, Math.min(FIT_ZOOM_MAX, z));
}

function render() {
  const zoom = fitZoom();
  const frame = rec && rec.length ? rec.frames[head] : null;
  const prev = rec && head > 0 ? rec.frames[head - 1] : null;
  panel.draw(frame, prev, state.width, state.height, info?.gray ?? false, { ...state, zoom }, state.pin);
  showPixel();
  logs.follow(head, $<HTMLInputElement>("logFollow").checked);
  renderStrips();
}

/** What moves with time even when the frame does not */
function renderStrips() {
  const m = sceneModel();
  for (const id of ["scenes", "sceneCard", "graphPane", "splitPanel"]) $(id).hidden = !m;
  drawScenes($<HTMLCanvasElement>("scenes"), m, rec?.end ?? 1, playTime);
  drawTimeline($("timeline"), rec, playTime, loop);
  drawPixelHistory($("history"), rec, state.pin, playTime);
  showFrameInfo();
  showScene(m);
}

function showScene(m: SceneModel | null) {
  if (!m) return;
  graph.draw(m, info?.scenes ?? [], playTime);
  const now = m.at(playTime);
  $("sceneNow").textContent = now.scene?.label ?? "-";
  const rows: [string, string][] = [];
  if (now.scene) rows.push(["for", `${((playTime - now.scene.t0) / 1000).toFixed(2)} s`]);
  if (now.arrival && now.arrival.from !== SCENE_NONE) rows.push(["came from", `${m.name(now.arrival.from)} · ${m.whyText(now.arrival)}`]);
  else if (now.arrival) rows.push(["came", now.arrival.why]);
  if (now.playing) rows.push([now.playing.kind, `${now.playing.label} · ${Math.round(playTime - now.playing.t0)}/${Math.round(now.playing.t1 - now.playing.t0)} ms`]);
  $("sceneInfo").innerHTML = rows.map(([k, v]) => `<dt>${k}</dt><dd></dd>`).join("");
  [...$("sceneInfo").querySelectorAll("dd")].forEach((dd, i) => (dd.textContent = rows[i][1]));
}

function showLogCount() {
  const total = rec?.logs.length ?? 0;
  $("logCount").textContent = total ? `${logs.count} of ${total} lines` : "LOG() / LOG_LVL() / printf from the C show here";
}

function showFrameInfo() {
  if (!rec || !rec.length) {
    $("frameInfo").textContent = "";
    return;
  }
  const t = rec.times[head];
  const dt = head ? t - rec.times[head - 1] : 0;
  const effect = rec.inEffect(head) ? " · effect" : "";
  const loopText = loop ? ` · loop ${loop[0]}–${loop[1]}` : "";
  $("frameInfo").textContent = `${Math.round(playTime)} ms · frame ${head + 1}/${rec.length} at ${t} ms, +${dt} ms${effect}${loopText}`;
}

function showPixel() {
  const pt = hover ?? state.pin;
  const dl = $("pixInfo");
  const canCopy = !!(pt && rec && rec.length);
  $<HTMLButtonElement>("copyRgb").disabled = !canCopy;
  $<HTMLButtonElement>("copyHsb").disabled = !canCopy;
  $("pixMode").textContent = hover ? "hover" : state.pin ? "pinned" : "hover, click to pin";
  if (!pt || !rec || !rec.length) {
    dl.innerHTML = "";
    return;
  }

  const raw = rec.raw(head, pt.x, pt.y);
  const rgb = decode(raw, rec.gray);
  const [h, s, b] = toHsb(rgb);
  const css = `rgb(${rgb.join(",")})`;
  const rows: [string, string][] = [
    ["x, y", `${pt.x}, ${pt.y}`],
    ["stored", `<span class="swatch" style="background:${css}"></span>${hex(raw)}`],
    ["RGB", rgb.join(", ")],
    ["HSB", `${h.toFixed(1)}°, ${s.toFixed(1)}, ${b.toFixed(1)}`],
    ["luma", luma(rgb).toFixed(1)],
  ];
  if (state.bright < 1)
    rows.push(["before LUT", `≈ ${rgb.map((v) => Math.min(255, Math.round(v / state.bright))).join(", ")}`]);
  dl.innerHTML = rows.map(([k, v]) => `<dt>${k}</dt><dd>${v}</dd>`).join("");
}

function showPanic(text?: string) {
  const box = $("panic");
  const p = rec?.panic;
  const message = text ?? (p ? `PANIC at ${p.file}:${p.line} (frame ${p.frame + 1})` : "");
  box.hidden = !message;
  box.textContent = message;
}

function status(text: string) {
  $("status").textContent = text;
}

// =================== Controls ===================

function heading(box: HTMLElement, text: string, hint = "") {
  const h = document.createElement("h3");
  h.className = "card-title side-title";
  h.textContent = text;
  h.title = hint;
  box.append(h);
}

/**
 * A live target: where a session starts - as the target does, or in an action of its -
 * then its inputs under their groups. Keys 1-9 press the first group's inputs.
 */
function buildLive(box: HTMLElement) {
  const { actions, inputs, inputGroups } = info!;
  if (actions.length) {
    heading(box, "start in", "a new session, from here: what was pressed is dropped");
    ["power-on", ...actions].forEach((name, i) => {
      const b = document.createElement("button");
      b.textContent = name;
      b.className = "start";
      if (!i) b.title = "as the board starts: Lab_Target_Init() and nothing else";
      b.onclick = () => restartLive(i - 1);
      box.append(b);
    });
  }

  const firstGroup = inputGroups[0] ?? "";
  keyInputs = inputs.map((_, i) => i).filter((i) => inputGroups[i] === firstGroup).slice(0, INPUT_KEYS.length);
  inputs.forEach((name, i) => {
    if (inputGroups[i] && inputGroups[i] !== inputGroups[i - 1]) heading(box, inputGroups[i]);
    const b = document.createElement("button");
    const key = keyInputs.indexOf(i);
    b.innerHTML = key >= 0 ? `<kbd>${INPUT_KEYS[key]}</kbd> ` : "";
    b.append(name);
    b.onclick = () => press(i);
    box.append(b);
  });
}

function buildActions() {
  const box = $("actions");
  box.innerHTML = "";
  $("restart").hidden = !info!.live;
  if (info!.live) {
    buildLive(box);
    return;
  }
  info!.actions.forEach((name, i) => {
    const b = document.createElement("button");
    b.textContent = name;
    b.onclick = () => startRun(i);
    box.append(b);
  });
  if (state.action >= info!.actions.length) state.action = 0;
}

function markActive() {
  if (info?.live) {
    [...$("actions").querySelectorAll(".start")].forEach((b, i) => b.classList.toggle("active", i - 1 === state.start));
    return;
  }
  [...$("actions").children].forEach((b, i) => b.classList.toggle("active", i === state.action));
}

function buildSizes() {
  const sel = $<HTMLSelectElement>("size");
  const fits = SIZES.filter(([w, h]) => w <= info!.maxWidth && h <= info!.maxHeight);
  sel.innerHTML = fits.map(([w, h]) => `<option value="${w}x${h}">${w}×${h}</option>`).join("");
  if (!fits.some(([w, h]) => w === state.width && h === state.height)) [state.width, state.height] = fits[fits.length - 1];
  sel.value = `${state.width}x${state.height}`;
}

function options(sel: HTMLSelectElement, values: readonly (string | number)[], label = (v: string | number) => String(v)) {
  sel.innerHTML = values.map((v) => `<option value="${v}">${label(v)}</option>`).join("");
}

function setLoopEnd(end: 0 | 1) {
  if (!rec || !rec.length) return;
  const l: [number, number] = loop ?? [0, rec.length - 1];
  l[end] = head;
  if (l[0] > l[1]) l.reverse();
  loop = l;
  render();
}

function copy(text: string) {
  navigator.clipboard?.writeText(text).then(() => status(`copied ${text}`));
}

function initControls() {
  $("target").textContent = __LAB_TARGET__;

  const variant = $<HTMLSelectElement>("variant");
  options(variant, VARIANTS);
  variant.value = state.variant;
  variant.onchange = () => {
    state.variant = variant.value as Variant;
    state.frame = 0;
    saveState();
    requestInfo();
  };

  $<HTMLSelectElement>("size").onchange = (e) => {
    [state.width, state.height] = (e.target as HTMLSelectElement).value.split("x").map(Number);
    state.pin = null;
    startRun(state.action);
  };

  const bright = $<HTMLInputElement>("bright");
  bright.value = String(state.bright);
  $("brightOut").textContent = `${Math.round(state.bright * 100)}%`;
  bright.oninput = () => ($("brightOut").textContent = `${Math.round(Number(bright.value) * 100)}%`);
  bright.onchange = () => {
    state.bright = Number(bright.value);
    startRun(state.action, head);
  };

  document.querySelectorAll<HTMLInputElement>("input[name=look]").forEach((r) => {
    r.checked = r.value === state.look;
    r.onchange = () => ((state.look = r.value as Look), saveState(), render());
  });
  for (const id of ["grid", "diff"] as const) {
    const box = $<HTMLInputElement>(id);
    box.checked = state[id];
    box.onchange = () => ((state[id] = box.checked), saveState(), render());
  }

  const zoom = $<HTMLSelectElement>("zoom");
  options(zoom, ZOOMS, (z) => (z ? `${z}×` : "fit"));
  zoom.value = String(state.zoom);
  zoom.onchange = () => ((state.zoom = Number(zoom.value)), saveState(), render());

  const speed = $<HTMLSelectElement>("speed");
  options(speed, SPEEDS, (s) => `${s}×`);
  speed.value = String(state.speed);
  speed.onchange = () => ((state.speed = Number(speed.value)), saveState());

  const logFilter = $<HTMLInputElement>("logFilter");
  const logLevel = $<HTMLSelectElement>("logLevel");
  logLevel.innerHTML = [LOG_SCENE, LOG_PRINT, ...LOG_LEVELS.keys()].map((l) => `<option value="${l}">${levelName(l)}</option>`).join("");
  const refilter = () => (logs.setFilter(logFilter.value, Number(logLevel.value)), showLogCount(), render());
  logFilter.oninput = refilter;
  logLevel.onchange = refilter;

  $("first").onclick = () => seek(0);
  $("back").onclick = () => (play(false), seek(head - 1));
  $("fwd").onclick = () => (play(false), seek(head + 1));
  $("play").onclick = () => play(!playing);
  $("loopA").onclick = () => setLoopEnd(0);
  $("loopB").onclick = () => setLoopEnd(1);
  $("restart").onclick = () => restartLive();

  for (const id of ["scenes", "timeline", "history"]) {
    const strip = $<HTMLCanvasElement>(id);
    const scrub = (e: MouseEvent) => rec && (play(false), seekTime(timeFromClick(e, strip, rec.end)));
    strip.onmousedown = (e) => scrub(e);
    strip.addEventListener("mousemove", (e) => e.buttons && scrub(e));
  }
  const scenes = $<HTMLCanvasElement>("scenes");
  scenes.addEventListener("mousemove", (e) => {
    if (!rec) return;
    const t = timeFromClick(e, scenes, rec.end);
    scenes.title = scenesTip(sceneModel(), t, (MARK_HOVER_PX / scenes.clientWidth) * rec.end);
  });

  const canvas = panel.canvas;
  canvas.onmousemove = (e) => ((hover = panel.pixelAt(e, fitZoom())), showPixel());
  canvas.onmouseleave = () => ((hover = null), showPixel());
  canvas.onclick = (e) => {
    const pt = panel.pixelAt(e, fitZoom());
    const same = pt && state.pin && pt.x === state.pin.x && pt.y === state.pin.y;
    state.pin = same ? null : pt;
    saveState();
    render();
  };

  const pixel = () => (hover ?? state.pin) && rec && rec.length ? rec.pixel(head, (hover ?? state.pin)!.x, (hover ?? state.pin)!.y) : null;
  $("copyRgb").onclick = () => {
    const p = pixel();
    if (p) copy(`WeUi_Gfx_Transform_Rgb2Color(${p.join(", ")})`);
  };
  $("copyHsb").onclick = () => {
    const p = pixel();
    if (!p) return;
    const [h, s, b] = toHsb(p).map((v) => v.toFixed(1));
    copy(`{.Hue = ${h}f, .Sat = ${s}f, .Bri = ${b}f}`);
  };

  document.onkeydown = (e) => {
    if (e.target instanceof HTMLInputElement || e.target instanceof HTMLSelectElement) return;
    const step = e.shiftKey ? STEP_BIG : 1;
    const keys: Record<string, () => void> = {
      " ": () => play(!playing),
      ArrowLeft: () => (play(false), seek(head - step)),
      ArrowRight: () => (play(false), seek(head + step)),
      Home: () => seek(0),
      End: () => rec && seek(rec.length - 1),
      a: () => setLoopEnd(0),
      b: () => setLoopEnd(1),
      Escape: () => ((loop = null), render()),
      d: () => $<HTMLInputElement>("diff").click(),
      r: () => info?.live && restartLive(),
      l: () => document.querySelector<HTMLInputElement>(`input[name=look][value=${state.look === "led" ? "flat" : "led"}]`)!.click(),
    };
    const inputKey = INPUT_KEYS.indexOf(e.key);
    if (inputKey >= 0 && inputKey < keyInputs.length) keys[e.key] = () => press(keyInputs[inputKey]);
    const fn = keys[e.key.length === 1 ? e.key.toLowerCase() : e.key];
    if (!fn) return;
    e.preventDefault();
    fn();
  };

  const resized = new ResizeObserver(() => render());
  resized.observe($("stage"));
  resized.observe($("graphPane"));
}

initControls();
initLayout(() => render());
requestInfo();
