// Stand-in backend for working on the UI in a plain browser, outside the app.
// Responses mimic the C++ handlers closely enough for layout work; they are not a spec.

type Listener = (payload: unknown) => void;

const startedAt = performance.now();
const listeners = new Map<string, Set<Listener>>();

setInterval(() => {
  const payload = { uptimeSeconds: (performance.now() - startedAt) / 1000 };
  listeners.get("app.heartbeat")?.forEach((fn) => fn(payload));
}, 1000);

function demoPeaks(bins: number): ArrayBuffer {
  // Decaying saw-ish tone, same shape as the backend demo.
  const peaks = new Float32Array(bins * 2);

  for (let i = 0; i < bins; i++) {
    const t = i / bins;
    const env = Math.exp(-4 * t) * Math.min(1, t * 200);
    const wobble = 0.85 + 0.15 * Math.sin(t * 40);
    peaks[i * 2] = -env * wobble;
    peaks[i * 2 + 1] = env * wobble;
  }

  return peaks.buffer;
}

export const mockBackend = {
  async call(method: string, params: unknown): Promise<unknown> {
    switch (method) {
      case "app.getInfo":
        return {
          ok: true,
          result: {
            product: location.port === "5174" ? "player" : "sampler",
            version: "dev",
            juceVersion: "(browser)",
            platform: navigator.platform,
            buildType: "Mock",
          },
        };
      case "app.ping":
        return { ok: true, result: { echo: (params as { payload: unknown })?.payload, serverTimeMs: Date.now() } };
      case "system.listMethods":
        return { ok: true, result: ["app.getInfo", "app.ping", "system.listMethods"] };
      default:
        return { ok: false, error: { code: "unknown_method", message: `Mock has no '${method}'` } };
    }
  },

  on(event: string, listener: Listener): () => void {
    const set = listeners.get(event) ?? new Set();
    set.add(listener);
    listeners.set(event, set);
    return () => set.delete(listener);
  },

  async fetchBinary(path: string): Promise<ArrayBuffer> {
    const url = new URL(path, location.href);

    if (url.pathname === "/api/demo/peaks") {
      return demoPeaks(Number(url.searchParams.get("bins") ?? 512));
    }

    throw new Error(`Mock has no resource '${path}'`);
  },
};
