<script lang="ts">
  // A horizontal slider bound to a C++ WebSliderRelay of the same name, so host automation
  // and the UI stay in sync.
  import { getSliderState } from "@juce-framework/webview";
  import { onDestroy } from "svelte";

  interface Props {
    name: string;
    label: string;
    format?: (scaled: number) => string;
  }

  let { name, label, format = (v) => v.toFixed(1) }: Props = $props();

  // The relay name is fixed for the slider's lifetime.
  // svelte-ignore state_referenced_locally
  const slider = getSliderState(name);

  let normalised = $state(slider.getNormalisedValue());
  let scaled = $state(slider.getScaledValue());
  let unit = $state(slider.properties.label);

  const valueListener = slider.valueChangedEvent.addListener(() => {
    normalised = slider.getNormalisedValue();
    scaled = slider.getScaledValue();
  });

  const propsListener = slider.propertiesChangedEvent.addListener(() => {
    unit = slider.properties.label;
    scaled = slider.getScaledValue();
  });

  onDestroy(() => {
    slider.valueChangedEvent.removeListener(valueListener);
    slider.propertiesChangedEvent.removeListener(propsListener);
  });

  function onInput(event: Event) {
    slider.setNormalisedValue(Number((event.currentTarget as HTMLInputElement).value));
  }
</script>

<label class="param">
  <span class="label">{label}</span>
  <input
    type="range"
    min="0"
    max="1"
    step="0.001"
    value={normalised}
    oninput={onInput}
    onpointerdown={() => slider.sliderDragStarted()}
    onpointerup={() => slider.sliderDragEnded()}
    onlostpointercapture={() => slider.sliderDragEnded()}
  />
  <span class="value mono">{format(scaled)} {unit}</span>
</label>

<style>
  .param {
    display: grid;
    grid-template-columns: 80px 1fr 84px;
    align-items: center;
    gap: var(--gap);
  }

  .label {
    color: var(--text-muted);
  }

  .value {
    text-align: right;
  }

  input[type="range"] {
    width: 100%;
    accent-color: var(--accent);
  }
</style>
