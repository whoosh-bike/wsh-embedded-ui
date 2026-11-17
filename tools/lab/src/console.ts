import { LOG_LEVELS, LOG_PRINT, LOG_SCENE, type LogLine } from "./protocol";

/**
 * What the C printed - LOG(), LOG_LVL(), printf - and the scene manager's steps, each line
 * with the frame it was drawn on and the virtual time. Lines ahead of the playhead are dimmed and the last one at or
 * before it is marked, so stepping through frames walks the log too. A click seeks.
 */

const SHOWN_MAX = 5000;

export const levelName = (level: number) =>
  level === LOG_SCENE ? "SCENE" : level === LOG_PRINT ? "PRINT" : (LOG_LEVELS[level] ?? "?");

export class LogConsole {
  private lines: LogLine[] = [];
  private rows: HTMLElement[] = [];
  private split = -1;
  private filter = "";
  private minLevel = LOG_SCENE;

  constructor(
    private readonly box: HTMLElement,
    private readonly onSeek: (frame: number) => void,
  ) {
    box.onclick = (e) => {
      const row = (e.target as HTMLElement).closest<HTMLElement>("[data-frame]");
      if (row) onSeek(Number(row.dataset.frame));
    };
  }

  setFilter(text: string, minLevel: number) {
    this.filter = text.toLowerCase();
    this.minLevel = minLevel;
    this.rebuild();
  }

  set(lines: LogLine[]) {
    this.lines = lines;
    this.rebuild();
  }

  /** The playhead moved: only rows between the old and the new split change state */
  follow(head: number, scroll: boolean) {
    const split = this.splitAt(head);
    if (split === this.split) return;
    const [from, to] = split > this.split ? [this.split + 1, split] : [split + 1, this.split];
    for (let i = Math.max(0, from); i <= to && i < this.rows.length; i++)
      this.rows[i].classList.toggle("future", i > split);
    this.rows[this.split]?.classList.remove("current");
    this.rows[split]?.classList.add("current");
    this.split = split;
    if (scroll) this.rows[split]?.scrollIntoView({ block: "nearest" });
  }

  get count() {
    return this.rows.length;
  }

  private shown(): LogLine[] {
    const shown = this.lines.filter(
      (l) =>
        (l.level === LOG_PRINT || l.level >= this.minLevel) && (!this.filter || l.text.toLowerCase().includes(this.filter)),
    );
    return shown.slice(-SHOWN_MAX);
  }

  private rebuild() {
    const frag = document.createDocumentFragment();
    this.rows = this.shown().map((l) => {
      const row = document.createElement("div");
      row.className = `log lvl-${levelName(l.level).toLowerCase()} future`;
      row.dataset.frame = String(l.frame);
      row.innerHTML = `<span class="muted">${l.frame + 1}</span><span class="muted">${l.time} ms</span><span>${levelName(l.level)}</span>`;
      const text = document.createElement("span");
      text.textContent = l.text;
      row.append(text);
      return row;
    });
    frag.append(...this.rows);
    this.box.replaceChildren(frag);
    this.split = -1;
  }

  /** Index of the last shown row printed at or before frame `head` */
  private splitAt(head: number): number {
    let lo = 0, hi = this.rows.length - 1, found = -1;
    while (lo <= hi) {
      const mid = (lo + hi) >> 1;
      if (Number(this.rows[mid].dataset.frame) <= head) (found = mid), (lo = mid + 1);
      else hi = mid - 1;
    }
    return found;
  }
}
