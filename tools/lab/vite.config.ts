import { execFile, execFileSync } from "node:child_process";
import { cpSync, existsSync, readFileSync } from "node:fs";
import { extname, resolve } from "node:path";
import { defineConfig, type Plugin, type ViteDevServer } from "vite";

/**
 * Builds the wasm with make before serving and again on every saved .c/.h of the
 * library or the target, then reloads the page - which replays the run it had open.
 * TARGET_DIR picks the target, as for make: `TARGET_DIR=<dir> npm run dev`.
 */

const LAB = import.meta.dirname;
const WEUI = resolve(LAB, "../..");
const TARGET_DIR = resolve(LAB, process.env.TARGET_DIR ?? "targets/demos");
const WASM_DIR = resolve(LAB, "build/wasm");
/* What the target.mk reads besides its own directory - a config it takes a flag from */
const targetDeps = (): string[] =>
  execFileSync("make", ["-s", "deps", `TARGET_DIR=${TARGET_DIR}`], { cwd: LAB, encoding: "utf8" })
    .split(/\s+/)
    .filter(Boolean);

const WATCHED = [
  ...["common", "external", "fs", "gfx", "host"].map((d) => resolve(WEUI, d)),
  resolve(LAB, "c"),
  TARGET_DIR,
  ...targetDeps(),
];
const REBUILD_DEBOUNCE_MS = 150;
const MAKE_JOBS = 8;
const MIME: Record<string, string> = { ".mjs": "text/javascript", ".wasm": "application/wasm" };

function targetName(): string {
  const mk = readFileSync(resolve(TARGET_DIR, "target.mk"), "utf8");
  return /^TARGET_NAME\s*:?=\s*(\S+)/m.exec(mk)?.[1] ?? "target";
}

/* The compiler's own lines: emcc and make repeat every command line around them */
const NOISE = /^(emcc: error: '|make(\[\d+\])?: )/;
const compilerOutput = (out: string) =>
  out.split("\n").filter((line) => !NOISE.test(line)).join("\n").trim();

function runMake(jobs: number): Promise<string | null> {
  return new Promise((done) =>
    execFile(
      "make",
      ["-s", `-j${jobs}`, `TARGET_DIR=${TARGET_DIR}`],
      { cwd: LAB, maxBuffer: 1 << 24 },
      (err, stdout, stderr) => done(err ? compilerOutput(`${stderr}${stdout}`) || String(err) : null),
    ),
  );
}

/* Parallel for speed; a failure is rebuilt serially, so variants do not interleave its message */
async function make(): Promise<string | null> {
  return (await runMake(MAKE_JOBS)) && runMake(1);
}

function weuiLab(): Plugin {
  let building: Promise<string | null> | null = null;
  let devServer: ViteDevServer | undefined;

  async function rebuild(server?: ViteDevServer) {
    const error = await (building ??= make().finally(() => (building = null)));
    if (!server) {
      if (error) throw new Error(error);
      return;
    }
    if (error) {
      server.config.logger.error(error);
      server.ws.send({ type: "error", err: { message: error, stack: "" } });
    } else {
      server.ws.send({ type: "full-reload" });
    }
  }

  return {
    name: "weui-lab",
    config: () => ({ define: { __LAB_TARGET__: JSON.stringify(targetName()) } }),
    async buildStart() {
      await rebuild(devServer);
    },
    configureServer(server) {
      devServer = server;
      let timer: ReturnType<typeof setTimeout> | undefined;
      server.watcher.add(WATCHED);
      server.watcher.on("change", (file) => {
        if (![".c", ".h", ".mk"].includes(extname(file))) return;
        clearTimeout(timer);
        timer = setTimeout(() => rebuild(server), REBUILD_DEBOUNCE_MS);
      });

      server.middlewares.use("/wasm", (req, res, next) => {
        const file = resolve(WASM_DIR, (req.url ?? "").split("?")[0].replace(/^\//, ""));
        if (!file.startsWith(WASM_DIR) || !existsSync(file)) return next();
        res.setHeader("Content-Type", MIME[extname(file)] ?? "application/octet-stream");
        res.setHeader("Cache-Control", "no-store");
        res.end(readFileSync(file));
      });
    },
    closeBundle() {
      if (existsSync(resolve(LAB, "dist"))) cpSync(WASM_DIR, resolve(LAB, "dist/wasm"), { recursive: true });
    },
  };
}

export default defineConfig({
  plugins: [weuiLab()],
  server: { open: true },
  worker: { format: "es" },
});
