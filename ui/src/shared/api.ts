// Typed contract between the frontends and the C++ backend.
// Keep in sync with the handlers registered in apps/*/Source.

export interface AppInfo {
  product: "sampler" | "player";
  version: string;
  juceVersion: string;
  platform: string;
  buildType: string;
}

export interface PingResult {
  echo: unknown;
  serverTimeMs: number;
}

/** RPC methods: name → params and result types. */
export interface Methods {
  "system.listMethods": { params: void; result: string[] };
  "app.getInfo": { params: void; result: AppInfo };
  "app.ping": { params: { payload: unknown }; result: PingResult };
}

export interface Heartbeat {
  uptimeSeconds: number;
}

/** Backend → UI events: name → payload type. */
export interface Events {
  "app.heartbeat": Heartbeat;
}

/** Binary resource endpoints (fetched with fetchBinary). */
export const endpoints = {
  /** Interleaved float32 [min0, max0, min1, max1, ...] for a demo waveform. */
  demoPeaks: (bins: number) => `/api/demo/peaks?bins=${bins}`,
} as const;
