import { getBackendResourceAddress, getNativeFunction } from "@juce-framework/webview";
import type { Events, Methods } from "./api";
import { mockBackend } from "./mockBackend";

const RPC_FUNCTION = "giclee_rpc"; // GicleeWebView::rpcFunctionName

/** False when the page runs in a plain browser (e.g. `npm run dev:sampler` without the app),
 *  in which case calls go to a mock backend so the UI can still be worked on. */
export const insideGiclee: boolean =
  window.__JUCE__?.initialisationData?.__juce__functions?.includes(RPC_FUNCTION) ?? false;

export class RpcError extends Error {
  constructor(
    readonly code: string,
    message: string,
  ) {
    super(message);
    this.name = "RpcError";
  }
}

interface RpcResponse {
  ok: boolean;
  result?: unknown;
  error?: { code: string; message: string };
}

const nativeRpc = insideGiclee ? getNativeFunction(RPC_FUNCTION) : null;

type ParamsArg<M extends keyof Methods> = Methods[M]["params"] extends void ? [] : [Methods[M]["params"]];

/** Calls a backend RPC method. Rejects with RpcError if the backend reports an error. */
export async function call<M extends keyof Methods>(method: M, ...params: ParamsArg<M>): Promise<Methods[M]["result"]> {
  const response = (nativeRpc
    ? await nativeRpc(method, params[0] ?? {})
    : await mockBackend.call(method, params[0])) as RpcResponse;

  if (!response.ok) {
    throw new RpcError(response.error?.code ?? "error", response.error?.message ?? "Unknown error");
  }

  return response.result as Methods[M]["result"];
}

/** Subscribes to a backend event. Returns an unsubscribe function. */
export function on<E extends keyof Events>(event: E, listener: (payload: Events[E]) => void): () => void {
  if (!insideGiclee) {
    return mockBackend.on(event, listener as (payload: unknown) => void);
  }

  const handle = window.__JUCE__.backend.addEventListener(event, listener as (payload: unknown) => void);
  return () => window.__JUCE__.backend.removeEventListener(handle);
}

/** Fetches a binary resource from the backend (e.g. waveform peaks). */
export async function fetchBinary(path: string): Promise<ArrayBuffer> {
  if (!insideGiclee) {
    return mockBackend.fetchBinary(path);
  }

  const response = await fetch(getBackendResourceAddress(path));

  if (!response.ok) {
    throw new Error(`GET ${path} failed: ${response.status}`);
  }

  return response.arrayBuffer();
}
