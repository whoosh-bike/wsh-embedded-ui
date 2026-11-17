import { sceneColor } from "./charts";
import type { Move, SceneModel } from "./scenes";

/**
 * The scenes as a graph of the moves the run actually made - not the ones the code could
 * make, which no run shows. A node per scene, an arrow per pair that was taken, thicker
 * the more often, and a click on one seeks to its next time. The scene at the playhead and
 * the move that brought it are lit; while that move's transition plays, its arrow runs.
 * Each arrow is labelled - why, with what, how often - and the moves are listed apart too.
 */

const SVG_NS = "http://www.w3.org/2000/svg";
const BEND = 0.18; /* of the chord: A->B and B->A bow apart */
const WIDTH_MAX = 4;
const LABEL_LINE_PX = 13;
/* In px whatever the stroke: a thick, often taken arrow keeps a small head */
const ARROW_PX = 9;

const MARGIN_X = 90;
const MARGIN_Y = 50;
const NODE_H = 26;
const CHAR_PX = 8.5;
const NODE_PAD_PX = 12;
const LOOP_R = 16;

type Pt = { x: number; y: number };

function el<K extends keyof SVGElementTagNameMap>(tag: K, attrs: Record<string, string | number>): SVGElementTagNameMap[K] {
  const e = document.createElementNS(SVG_NS, tag);
  for (const [k, v] of Object.entries(attrs)) e.setAttribute(k, String(v));
  return e;
}

/** Where the segment from the node's centre towards `to` leaves its box */
function edgeOf(c: Pt, halfW: number, halfH: number, to: Pt): Pt {
  const dx = to.x - c.x, dy = to.y - c.y;
  const k = Math.min(halfW / Math.abs(dx || 1e-9), halfH / Math.abs(dy || 1e-9));
  return { x: c.x + dx * k, y: c.y + dy * k };
}

export class SceneGraph {
  private key = "";

  constructor(
    private readonly svg: SVGSVGElement,
    private readonly list: HTMLElement,
    private readonly onPick: (move: Move) => void,
  ) {}

  /** Redrawn only when the moves, the scene, the lit move or the size change */
  draw(model: SceneModel | null, scenes: string[], t: number) {
    const now = model?.at(t);
    const lit = now?.lastMove ?? null;
    const running = !!lit && now?.playing?.kind === "transition" && lit.times.includes(now.playing.t0);
    const width = this.svg.clientWidth;
    const height = this.svg.clientHeight;
    const key = [model?.moves.map((m) => m.times.length).join(), now?.scene?.scene, model?.moves.indexOf(lit!), running, width, height].join("|");
    if (key === this.key) return;
    this.key = key;

    this.svg.replaceChildren();
    this.list.replaceChildren();
    if (!model || !width || !height) return;

    this.svg.setAttribute("viewBox", `0 0 ${width} ${height}`);
    const defs = el("defs", {});
    for (const [id, cls] of [["arrow", "arrow"], ["arrowLit", "arrow lit"]]) {
      const marker = el("marker", {
        id,
        viewBox: "0 0 10 10",
        refX: 9,
        refY: 5,
        markerUnits: "userSpaceOnUse",
        markerWidth: ARROW_PX,
        markerHeight: ARROW_PX,
        orient: "auto-start-reverse",
      });
      marker.append(el("path", { d: "M0,0 L10,5 L0,10 z", class: cls }));
      defs.append(marker);
    }
    this.svg.append(defs);

    const count = scenes.length || Math.max(-1, ...model.moves.flatMap((m) => [m.from, m.to])) + 1;
    const rx = Math.max(0, width / 2 - MARGIN_X), ry = Math.max(0, height / 2 - MARGIN_Y);
    const pos: Pt[] = Array.from({ length: count }, (_, i) => {
      const a = -Math.PI / 2 + (2 * Math.PI * i) / count;
      return { x: width / 2 + rx * Math.cos(a), y: height / 2 + ry * Math.sin(a) };
    });
    const halfW = (i: number) => (model.name(i).length * CHAR_PX) / 2 + NODE_PAD_PX;

    const pairs = new Map<string, Move[]>();
    for (const m of model.moves) {
      const k = `${m.from}>${m.to}`;
      pairs.set(k, [...(pairs.get(k) ?? []), m]);
    }
    const most = Math.max(1, ...[...pairs.values()].map((ms) => ms.reduce((n, m) => n + m.times.length, 0)));
    const labels = el("g", { class: "labels" });

    for (const ms of pairs.values()) {
      const { from, to } = ms[0];
      if (from >= count || to >= count) continue;
      const taken = ms.reduce((n, m) => n + m.times.length, 0);
      const isLit = !!lit && ms.includes(lit);
      const lines = ms.map((m) => `${model.whyText(m)} ×${m.times.length}`);
      let path: SVGPathElement;
      let labelAt: Pt;
      if (from === to) {
        const c = pos[from], top = { x: c.x, y: c.y - NODE_H / 2 };
        path = el("path", { d: `M${top.x - LOOP_R / 2},${top.y} a${LOOP_R},${LOOP_R} 0 1,1 ${LOOP_R},0` });
        labelAt = { x: c.x, y: top.y - 2 * LOOP_R - (lines.length - 1) * LABEL_LINE_PX - 4 };
      } else {
        const a = pos[from], b = pos[to];
        const mid = { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 };
        const ctrl = { x: mid.x - (b.y - a.y) * BEND, y: mid.y + (b.x - a.x) * BEND };
        const p0 = edgeOf(a, halfW(from), NODE_H / 2, ctrl), p1 = edgeOf(b, halfW(to), NODE_H / 2, ctrl);
        path = el("path", { d: `M${p0.x},${p0.y} Q${ctrl.x},${ctrl.y} ${p1.x},${p1.y}` });
        labelAt = { x: 0.25 * p0.x + 0.5 * ctrl.x + 0.25 * p1.x, y: 0.25 * p0.y + 0.5 * ctrl.y + 0.25 * p1.y - ((lines.length - 1) * LABEL_LINE_PX) / 2 };
      }
      path.setAttribute("class", `edge${isLit ? " lit" : ""}${isLit && running ? " running" : ""}`);
      path.setAttribute("stroke-width", String(1 + ((WIDTH_MAX - 1) * taken) / most));
      path.setAttribute("marker-end", `url(#${isLit ? "arrowLit" : "arrow"})`);
      const title = el("title", {});
      title.textContent = ms.map((m) => `${model.name(m.from)} → ${model.name(m.to)}: ${model.whyText(m)} ×${m.times.length}`).join("\n");
      path.append(title);
      path.onclick = () => this.onPick(ms[0]);
      this.svg.append(path);

      ms.forEach((m, i) => {
        const text = el("text", { x: labelAt.x, y: labelAt.y + i * LABEL_LINE_PX, class: `label${m === lit ? " lit" : ""}` });
        text.textContent = lines[i];
        text.onclick = () => this.onPick(m);
        labels.append(text);
      });
    }

    for (let i = 0; i < count; i++) {
      const g = el("g", { class: `node${now?.scene?.scene === i ? " lit" : ""}` });
      const w = 2 * halfW(i);
      g.append(el("rect", { x: pos[i].x - w / 2, y: pos[i].y - NODE_H / 2, width: w, height: NODE_H, rx: 6, fill: sceneColor(i) }));
      const text = el("text", { x: pos[i].x, y: pos[i].y + 1 });
      text.textContent = model.name(i);
      g.append(text);
      this.svg.append(g);
    }
    /* Over the nodes, so a label crossing one stays readable */
    this.svg.append(labels);

    for (const m of [...model.moves].sort((a, b) => a.from - b.from || a.to - b.to)) {
      const row = document.createElement("div");
      row.className = `move${m === lit ? " lit" : ""}`;
      row.title = "seek to its next time";
      row.innerHTML = `<span></span><span class="muted"></span><span class="muted">×${m.times.length}</span>`;
      row.children[0].textContent = `${model.name(m.from)} → ${model.name(m.to)}`;
      row.children[1].textContent = model.whyText(m);
      row.onclick = () => this.onPick(m);
      this.list.append(row);
    }
  }
}
