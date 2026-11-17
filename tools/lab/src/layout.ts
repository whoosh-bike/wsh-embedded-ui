/**
 * The page's proportions, set by dragging the bars between its parts: the side columns'
 * widths, the console's height, and how the middle is shared between the panel and the
 * scene graph. Dragged to the end, a part folds away; a double-click puts a bar back.
 * Kept per browser, not per session: it is how the person likes the page, not the run.
 */

const KEY = "weui-lab:layout";
const FOLD_PX = 60;
const FOLD_SHARE = 0.05;
const SIDE_MAX_SHARE = 0.45;
const CONSOLE_MAX_SHARE = 0.7;

interface Layout {
  actsW: number;
  inspW: number;
  consoleH: number;
  /** Of the middle's height, the panel's; the graph has the rest */
  panelShare: number;
}

const DEFAULTS: Layout = { actsW: 210, inspW: 340, consoleH: 150, panelShare: 0.55 };

function load(): Layout {
  try {
    return { ...DEFAULTS, ...JSON.parse(localStorage.getItem(KEY) ?? "{}") };
  } catch {
    return { ...DEFAULTS };
  }
}

function save(layout: Layout) {
  try {
    localStorage.setItem(KEY, JSON.stringify(layout));
  } catch {
    /* a private window keeps no layout: the defaults come back next time */
  }
}

const clamp = (v: number, lo: number, hi: number) => Math.max(lo, Math.min(hi, v));
const fold = (px: number) => (px < FOLD_PX ? 0 : px);

export function initLayout(onChange: () => void) {
  const layout = load();
  const root = document.documentElement.style;
  const $ = (id: string) => document.getElementById(id)!;

  const apply = () => {
    root.setProperty("--acts-w", `${layout.actsW}px`);
    root.setProperty("--insp-w", `${layout.inspW}px`);
    root.setProperty("--console-h", `${layout.consoleH}px`);
    /* Grow factors under 1 would leave a lone panel - no graph to share with - short of
     * the space: in per cent they add up to 100, and one alone still takes it all */
    $("stage").style.flex = `${layout.panelShare * 100} 1 0`;
    $("graphPane").style.flex = `${(1 - layout.panelShare) * 100} 1 0`;
    $("stage").classList.toggle("folded", layout.panelShare === 0);
    $("graphPane").classList.toggle("folded", layout.panelShare === 1);
    onChange();
  };

  const bar = (id: string, field: keyof Layout, at: (e: PointerEvent, start: Layout, from: PointerEvent) => number) => {
    const el = $(id);
    el.title = "drag to resize, double-click to reset";
    el.onpointerdown = (down) => {
      const start = { ...layout };
      el.setPointerCapture(down.pointerId);
      el.classList.add("dragging");
      el.onpointermove = (e) => {
        layout[field] = at(e, start, down);
        apply();
      };
      el.onpointerup = () => {
        el.onpointermove = null;
        el.classList.remove("dragging");
        save(layout);
      };
    };
    el.ondblclick = () => {
      layout[field] = DEFAULTS[field];
      apply();
      save(layout);
    };
  };

  bar("splitActs", "actsW", (e) => fold(clamp(e.clientX, 0, innerWidth * SIDE_MAX_SHARE)));
  bar("splitInsp", "inspW", (e) => fold(clamp(innerWidth - e.clientX, 0, innerWidth * SIDE_MAX_SHARE)));
  bar("splitFoot", "consoleH", (e, start, from) => fold(clamp(start.consoleH + from.clientY - e.clientY, 0, innerHeight * CONSOLE_MAX_SHARE)));
  bar("splitPanel", "panelShare", (e) => {
    const r = $("split").getBoundingClientRect();
    const share = clamp((e.clientY - r.top) / r.height, 0, 1);
    return share < FOLD_SHARE ? 0 : share > 1 - FOLD_SHARE ? 1 : share;
  });

  apply();
}
