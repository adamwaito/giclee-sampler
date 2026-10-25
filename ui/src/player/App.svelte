<script lang="ts">
  // Phase 1 shell: plugin info over RPC and one host-automatable parameter via a slider relay.
  import type { AppInfo } from "$shared/api";
  import { call } from "$shared/bridge";
  import ParamSlider from "$shared/ParamSlider.svelte";
  import { onMount } from "svelte";

  let info = $state<AppInfo | null>(null);

  onMount(() => {
    call("app.getInfo").then((i) => (info = i));
  });
</script>

<div class="shell">
  <header>
    <div class="brand">Giclee <span>Player</span></div>
    {#if info}
      <div class="meta mono">v{info.version}</div>
    {/if}
  </header>

  <main>
    <section class="card empty">
      <h2>Sample set</h2>
      <p>No sample set loaded.</p>
    </section>

    <section class="card">
      <h2>Output</h2>
      <ParamSlider name="gain" label="Volume" />
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
    padding: 10px 16px;
    border-bottom: 1px solid var(--border);
  }

  .brand {
    font-size: 14px;
    font-weight: 700;
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
    padding: 16px;
  }

  .empty p {
    margin: 0;
    color: var(--text-muted);
  }
</style>
