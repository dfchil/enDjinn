#!/usr/bin/env node

import { spawn } from "node:child_process";
import { createReadStream, existsSync, mkdtempSync, rmSync } from "node:fs";
import { createServer } from "node:http";
import { tmpdir } from "node:os";
import { basename, dirname, extname, resolve, sep } from "node:path";

const root = resolve(dirname(new URL(import.meta.url).pathname), "..");
const defaultHtml = resolve(
  root, "examples/enj_deep_modifiers/build/web-endjinn/enj_deep_modifiers.html");
const html = resolve(process.argv[2] ?? defaultHtml);

const chromeCandidates = [
  process.env.CHROME,
  "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
  "/Applications/Chromium.app/Contents/MacOS/Chromium",
  "/usr/bin/google-chrome",
  "/usr/bin/chromium",
  "/usr/bin/chromium-browser",
].filter(Boolean);
const chrome = chromeCandidates.find(existsSync);
if (!chrome) {
  throw new Error("Chrome/Chromium not found; set CHROME=/path/to/browser");
}
if (!existsSync(html)) {
  throw new Error(`Web example not found: ${html}`);
}

const delay = milliseconds => new Promise(resolveDelay =>
  setTimeout(resolveDelay, milliseconds));

class Cdp {
  constructor(url) {
    this.socket = new WebSocket(url);
    this.nextId = 1;
    this.pending = new Map();
    this.events = [];
  }

  async open() {
    await new Promise((resolveOpen, reject) => {
      this.socket.addEventListener("open", resolveOpen, { once: true });
      this.socket.addEventListener("error", reject, { once: true });
    });
    this.socket.addEventListener("message", event => {
      const message = JSON.parse(event.data);
      const resolveMessage = this.pending.get(message.id);
      if (resolveMessage) {
        this.pending.delete(message.id);
        resolveMessage(message);
      } else if (message.method) {
        this.events.push(message);
      }
    });
  }

  send(method, params = {}) {
    const id = this.nextId++;
    return new Promise((resolveMessage, reject) => {
      const timer = setTimeout(() => {
        this.pending.delete(id);
        reject(new Error(`CDP timeout: ${method}`));
      }, 15000);
      this.pending.set(id, message => {
        clearTimeout(timer);
        if (message.error) {
          reject(new Error(`${method}: ${message.error.message}`));
        } else {
          resolveMessage(message.result);
        }
      });
      this.socket.send(JSON.stringify({ id, method, params }));
    });
  }

  close() {
    this.socket.close();
  }
}

function serve(directory) {
  const contentTypes = new Map([
    [".data", "application/octet-stream"],
    [".html", "text/html; charset=utf-8"],
    [".js", "text/javascript; charset=utf-8"],
    [".wasm", "application/wasm"],
  ]);
  return createServer((request, response) => {
    const pathname = decodeURIComponent(
      new URL(request.url, "http://127.0.0.1").pathname);
    if (pathname === "/favicon.ico") {
      response.writeHead(204).end();
      return;
    }
    const relative = pathname === "/" ? basename(html) : pathname.slice(1);
    const file = resolve(directory, relative);
    if (file !== directory && !file.startsWith(directory + sep)) {
      response.writeHead(403).end();
      return;
    }
    const stream = createReadStream(file);
    stream.on("error", () => response.writeHead(404).end());
    response.setHeader("Content-Type",
      contentTypes.get(extname(file)) ?? "application/octet-stream");
    stream.pipe(response);
  });
}

async function evaluate(cdp, expression) {
  const result = await cdp.send("Runtime.evaluate", {
    expression,
    awaitPromise: true,
    returnByValue: true,
  });
  if (result.exceptionDetails) {
    throw new Error(result.exceptionDetails.text);
  }
  return result.result.value;
}

async function canvasStats(cdp) {
  const box = JSON.parse(await evaluate(cdp,
    "JSON.stringify((()=>{const r=Module.canvas.getBoundingClientRect();" +
    "return{x:r.x,y:r.y,width:r.width,height:r.height,scale:devicePixelRatio}})())"));
  const shot = await cdp.send("Page.captureScreenshot", {
    format: "png",
    clip: {
      x: box.x,
      y: box.y,
      width: box.width,
      height: box.height,
      scale: box.scale,
    },
  });
  const source = JSON.stringify(`data:image/png;base64,${shot.data}`);
  return JSON.parse(await evaluate(cdp,
    `(async()=>{const image=new Image();await new Promise((ok,bad)=>{` +
    `image.onload=ok;image.onerror=bad;image.src=${source}});` +
    `const canvas=document.createElement("canvas");canvas.width=image.width;` +
    `canvas.height=image.height;const context=canvas.getContext("2d");` +
    `context.drawImage(image,0,0);const pixels=context.getImageData(` +
    `0,0,canvas.width,canvas.height).data;let nonBackground=0,cyan=0,orange=0;` +
    `let brightness=0;const colors=new Set();for(let i=0;i<pixels.length;i+=4){` +
    `const r=pixels[i],g=pixels[i+1],b=pixels[i+2];brightness+=r+g+b;` +
    `if(r+g+b>80)nonBackground++;if(r+g+b>80&&b>r*1.2&&g>r*1.1)cyan++;` +
    `if(r>120&&r>g*1.15&&g>b*1.3)orange++;` +
    `colors.add((r>>4)|((g>>4)<<4)|((b>>4)<<8))}` +
    `return JSON.stringify({width:canvas.width,height:canvas.height,` +
    `nonBackground,cyan,orange,brightness,colors:colors.size})})()`));
}

async function press(cdp, key, code, keyCode) {
  await cdp.send("Input.dispatchKeyEvent", {
    type: "keyDown", key, code, windowsVirtualKeyCode: keyCode,
  });
  await delay(80);
  await cdp.send("Input.dispatchKeyEvent", {
    type: "keyUp", key, code, windowsVirtualKeyCode: keyCode,
  });
  await delay(250);
}

const server = serve(dirname(html));
const profile = mkdtempSync(resolve(tmpdir(), "enj-web-regression-"));
let browser;
let cdp;

try {
  await new Promise(resolveListen =>
    server.listen(0, "127.0.0.1", resolveListen));
  const { port } = server.address();
  const url = `http://127.0.0.1:${port}/${basename(html)}`;

  browser = spawn(chrome, [
    "--headless=new",
    "--no-first-run",
    "--no-default-browser-check",
    "--disable-background-networking",
    "--disable-component-update",
    "--disable-default-apps",
    "--disable-extensions",
    "--disable-sync",
    "--enable-unsafe-swiftshader",
    "--use-angle=swiftshader",
    "--window-size=1280,960",
    "--remote-debugging-port=0",
    `--user-data-dir=${profile}`,
    "about:blank",
  ], { stdio: ["ignore", "ignore", "pipe"] });

  let browserLog = "";
  const browserWebSocket = await new Promise((resolveSocket, reject) => {
    const timer = setTimeout(() => reject(new Error(
      `Chrome did not expose DevTools:\n${browserLog}`)), 15000);
    browser.on("error", reject);
    browser.stderr.on("data", chunk => {
      browserLog += chunk;
      const match = browserLog.match(/DevTools listening on (ws:\/\/[^\s]+)/);
      if (match) {
        clearTimeout(timer);
        resolveSocket(match[1]);
      }
    });
  });

  const endpoint = new URL(browserWebSocket);
  let page;
  for (let attempt = 0; attempt < 100 && !page; attempt++) {
    const targets = await (await fetch(
      `http://${endpoint.host}/json/list`)).json();
    page = targets.find(target => target.type === "page");
    if (!page) await delay(50);
  }
  if (!page) throw new Error("Chrome page target not found");

  cdp = new Cdp(page.webSocketDebuggerUrl);
  await cdp.open();
  await cdp.send("Page.enable");
  await cdp.send("Runtime.enable");
  await cdp.send("Log.enable");
  await cdp.send("Page.navigate", { url });

  let readyStats;
  const deadline = Date.now() + 30000;
  while (Date.now() < deadline) {
    try {
      const ready = await evaluate(cdp,
        "typeof Module==='object'&&Module.canvas&&" +
        "!!Module.canvas.getContext('webgl2')");
      if (ready) {
        readyStats = await canvasStats(cdp);
        if (readyStats.width >= 600 && readyStats.height >= 400 &&
            readyStats.nonBackground > 1000 && readyStats.colors > 8) {
          await delay(300);
          readyStats = await canvasStats(cdp);
          break;
        }
      }
    } catch {
      // Navigation replaces the execution context during startup.
    }
    await delay(100);
  }
  if (!readyStats || readyStats.width < 600 || readyStats.height < 400 ||
      readyStats.nonBackground <= 1000) {
    throw new Error("WebGL canvas did not produce a rendered frame");
  }

  const modes = { opaque_on: readyStats };
  await press(cdp, "c", "KeyC", 67);
  modes.opaque_off = await canvasStats(cdp);
  await press(cdp, "x", "KeyX", 88);
  modes.translucent_off = await canvasStats(cdp);
  await press(cdp, "c", "KeyC", 67);
  modes.translucent_on = await canvasStats(cdp);
  await press(cdp, "x", "KeyX", 88);
  modes.mixed_on = await canvasStats(cdp);
  await press(cdp, "c", "KeyC", 67);
  modes.mixed_off = await canvasStats(cdp);

  for (const [name, stats] of Object.entries(modes)) {
    if (stats.nonBackground <= 1000 || stats.colors <= 8) {
      throw new Error(`${name}: implausible canvas statistics ${JSON.stringify(stats)}`);
    }
    console.log(`  ${name}: ${JSON.stringify(stats)}`);
  }
  for (const mode of ["opaque", "translucent", "mixed"]) {
    const on = modes[`${mode}_on`];
    const off = modes[`${mode}_off`];
    if (on.cyan <= off.cyan + 250) {
      throw new Error(`${mode}: modifier visualization toggle had no visible effect`);
    }
  }
  const receiverBrightness = [modes.opaque_off.brightness,
    modes.translucent_off.brightness, modes.mixed_off.brightness];
  if (new Set(receiverBrightness).size !== receiverBrightness.length) {
    throw new Error("receiver transparency modes produced identical output");
  }

  const fatalEvents = cdp.events.filter(event =>
    event.method === "Runtime.exceptionThrown" ||
    (event.method === "Log.entryAdded" &&
      event.params.entry.level === "error"));
  if (fatalEvents.length) {
    throw new Error(`browser errors: ${JSON.stringify(fatalEvents)}`);
  }
  console.log("web_modifier_visual_regression: ok");
} finally {
  cdp?.close();
  browser?.kill("SIGTERM");
  server.close();
  rmSync(profile, { recursive: true, force: true });
}
