import path from "node:path";
import {fileURLToPath} from "node:url";

import {storybookTest} from "@storybook/experimental-addon-test/vitest-plugin";
import {defineWorkspace} from "vitest/config";


const dirname =
  "undefined" !== typeof __dirname ?
      __dirname :
      path.dirname(fileURLToPath(import.meta.url));

export default defineWorkspace([
    "vite.config.ts",
    {
        extends: "vite.config.ts",
        plugins: [
            storybookTest({configDir: path.join(dirname, ".storybook")}),
        ],
        test: {
            name: "storybook",
            browser: {
                enabled: true,
                headless: true,
                instances: [
                    {browser: "chromium"},
                ],
                provider: "playwright",
            },
            setupFiles: [".storybook/vitest.setup.ts"],
        },
    },
]);
