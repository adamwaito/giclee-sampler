import { svelte } from "@sveltejs/vite-plugin-svelte";
import { fileURLToPath } from "node:url";
import { defineConfig } from "vite";

// One Vite project, two frontends. The mode selects which: `vite --mode sampler|player`.
const devPorts = { sampler: 5173, player: 5174 } as const;
type AppName = keyof typeof devPorts;

const here = (path: string) => fileURLToPath(new URL(path, import.meta.url));

export default defineConfig(({ mode }) => {
  // Tools such as svelte-check load this config in the default mode; treat that as the sampler.
  const app: AppName = mode in devPorts ? (mode as AppName) : "sampler";

  return {
    root: here(`./src/${app}`),
    // Served from the WebView resource provider root, so asset URLs must be relative.
    base: "./",
    plugins: [svelte({ configFile: here("./svelte.config.js") })],
    resolve: {
      alias: { $shared: here("./src/shared") },
    },
    server: { port: devPorts[app], strictPort: true },
    build: {
      // CMake passes GICLEE_UI_OUT_DIR so build output stays inside the build tree.
      outDir: process.env.GICLEE_UI_OUT_DIR ?? here(`./dist/${app}`),
      emptyOutDir: true,
      // WKWebView follows the system Safari version; WebView2 is evergreen Chromium.
      target: ["safari15", "chrome110"],
    },
  };
});
