import type {ThemeConfig} from "antd";


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


export default THEME_CONFIG;
