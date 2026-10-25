<script lang="ts">
  // Draws interleaved [min, max] peak pairs, one vertical stroke per device pixel column.
  interface Props {
    peaks: Float32Array | null;
    height?: number;
  }

  let { peaks, height = 140 }: Props = $props();

  let canvas: HTMLCanvasElement;
  let width = $state(0);

  function draw() {
    if (!canvas) return;

    const dpr = window.devicePixelRatio || 1;
    const w = Math.max(1, Math.round(width * dpr));
    const h = Math.max(1, Math.round(height * dpr));
    canvas.width = w;
    canvas.height = h;

    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const style = getComputedStyle(canvas);
    ctx.clearRect(0, 0, w, h);

    const mid = h / 2;
    ctx.fillStyle = style.getPropertyValue("--border");
    ctx.fillRect(0, Math.floor(mid), w, Math.max(1, Math.round(dpr)));

    if (!peaks || peaks.length < 2) return;

    const bins = peaks.length / 2;
    ctx.fillStyle = style.getPropertyValue("--accent");

    for (let x = 0; x < w; x++) {
      // Bins covered by this pixel column; at least one.
      const from = Math.floor((x * bins) / w);
      const to = Math.max(from + 1, Math.floor(((x + 1) * bins) / w));
      let lo = 1;
      let hi = -1;

      for (let b = from; b < to && b < bins; b++) {
        lo = Math.min(lo, peaks[b * 2]);
        hi = Math.max(hi, peaks[b * 2 + 1]);
      }

      const top = mid - hi * mid;
      const bottom = mid - lo * mid;
      ctx.fillRect(x, top, 1, Math.max(1, bottom - top));
    }
  }

  $effect(() => {
    // Re-draw when the data or size changes.
    void peaks;
    void width;
    void height;
    draw();
  });
</script>

<div class="waveform" bind:clientWidth={width} style:height="{height}px">
  <canvas bind:this={canvas} style:width="{width}px" style:height="{height}px"></canvas>
</div>

<style>
  .waveform {
    width: 100%;
    background: var(--bg);
    border-radius: 6px;
    overflow: hidden;
  }

  canvas {
    display: block;
  }
</style>
