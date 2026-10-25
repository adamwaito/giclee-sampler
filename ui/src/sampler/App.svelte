<script lang="ts">
  // Phase 1 shell: proves the three bridge channels (RPC, events, binary resources) work.
  import type { AppInfo } from "$shared/api";
  import { endpoints } from "$shared/api";
  import { call, fetchBinary, insideGiclee, on } from "$shared/bridge";
  import Waveform from "$shared/Waveform.svelte";
  import { onMount } from "svelte";

  let info = $state<AppInfo | null>(null);
  let pingMs = $state<number | null>(null);
  let uptime = $state<number | null>(null);
  let peaks = $state<Float32Array | null>(null);
  let peakBytes = $state(0);
  let error = $state<string | null>(null);

  async function ping() {
    const started = performance.now();
    await call("app.ping", { payload: "hello" });
    pingMs = performance.now() - started;
  }

  async function loadPeaks() {
    const buffer = await fetchBinary(endpoints.demoPeaks(2048));
    peakBytes = buffer.byteLength;
    peaks = new Float32Array(buffer);
  }

  onMount(() => {
    const off = on("app.heartbeat", (hb) => (uptime = hb.uptimeSeconds));

    Promise.all([call("app.getInfo").then((i) => (info = i)), ping(), loadPeaks()]).catch(
      (e: unknown) => (error = e instanceof Error ? e.message : String(e)),
    );

    return off;
  });
</script>

<div class="shell">
  <header>
    <div class="brand">Giclee <span>Sampler</span></div>
    {#if info}
      <div class="meta mono">v{info.version} · {info.platform} · {info.buildType}</div>
    {/if}
  </header>

  <main>
    {#if !insideGiclee}
      <p class="notice">Running in a browser with a mock backend. Start the app to talk to C++.</p>
    {/if}
    {#if error}
      <p class="notice bad">{error}</p>
    {/if}

    <section class="card">
      <h2>Bridge</h2>
      <dl>
        <dt>RPC round-trip</dt>
        <dd class="mono">{pingMs === null ? "…" : `${pingMs.toFixed(2)} ms`}</dd>
        <dt>Events</dt>
        <dd class="mono">{uptime === null ? "waiting…" : `heartbeat · up ${uptime.toFixed(0)} s`}</dd>
        <dt>Binary</dt>
        <dd class="mono">{peakBytes ? `${peakBytes.toLocaleString()} bytes of peaks` : "…"}</dd>
        <dt>Engine</dt>
        <dd class="mono">{info?.juceVersion ?? "…"}</dd>
      </dl>
      <button onclick={ping}>Ping again</button>
    </section>

    <section class="card">
      <h2>Waveform (demo signal from giclee_dsp)</h2>
      <Waveform {peaks} />
    </section>
  </main>
</div>

<style>
  .shell {
    display: flex;
    flex-direction: column;
    height: 100%;
  }

  header {
    display: flex;
    align-items: baseline;
    justify-content: space-between;
    padding: 12px 20px;
    border-bottom: 1px solid var(--border);
  }

  .brand {
    font-size: 15px;
    font-weight: 700;
    letter-spacing: 0.02em;
  }

  .brand span {
    color: var(--accent);
    font-weight: 500;
  }

  .meta {
    color: var(--text-muted);
    font-size: 12px;
  }

  main {
    display: grid;
    gap: var(--gap);
    padding: 20px;
    max-width: 960px;
    width: 100%;
    margin: 0 auto;
  }

  dl {
    display: grid;
    grid-template-columns: 140px 1fr;
    gap: 6px var(--gap);
    margin: 0 0 12px;
  }

  dt {
    color: var(--text-muted);
  }

  dd {
    margin: 0;
  }

  .notice {
    margin: 0;
    padding: 8px 12px;
    border-radius: 6px;
    background: var(--surface-raised);
    color: var(--warn);
  }

  .notice.bad {
    color: var(--bad);
  }
</style>
