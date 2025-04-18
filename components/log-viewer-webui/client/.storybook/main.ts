import type {StorybookConfig} from "@storybook/react-vite";


const config: StorybookConfig = {
    stories: [
        "../src/**/*.stories.@(ts|tsx)",
    ],
    addons: [
        "@storybook/addon-essentials",
        "@chromatic-com/storybook",
        "@storybook/experimental-addon-test",
    ],
    framework: {
        name: "@storybook/react-vite",
        options: {},
    },
};


export default config;
