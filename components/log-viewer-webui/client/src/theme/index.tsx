import type {ThemeConfig} from "antd/es/config-provider/context";

import useThemeStore from "./themeStore";
import {THEME_MODE} from "./typings";


/**
 * Ant Design theme configuration.
 */
const THEME_CONFIG: ThemeConfig = Object.freeze({
    token: {
        fontFamily: "'Inter', sans-serif",
        colorPrimary: "#2a8efa",
        borderRadius: 3,
    },
    cssVar: true,
    hashed: false,
});


export {
    THEME_MODE,
    useThemeStore,
};
export default THEME_CONFIG;
