import React, {useCallback} from "react";

import {Menu} from "antd";

import DarkModeOutlinedIcon from "@mui/icons-material/DarkModeOutlined";
import LightModeOutlinedIcon from "@mui/icons-material/LightModeOutlined";
import SettingsBrightnessOutlinedIcon from "@mui/icons-material/SettingsBrightnessOutlined";

import {
    THEME_MODE,
    useThemeStore,
} from "../../theme";


const {Item} = Menu;

/**
 * Maps theme mode to icon.
 */
const THEME_MODE_TO_ICON: Record<THEME_MODE, React.ReactNode> = Object.freeze({
    [THEME_MODE.SYSTEM]: <SettingsBrightnessOutlinedIcon/>,
    [THEME_MODE.DARK]: <DarkModeOutlinedIcon/>,
    [THEME_MODE.LIGHT]: <LightModeOutlinedIcon/>,
});

/**
 * Maps theme mode to label.
 */
const THEME_MODE_TO_LABEL: Record<THEME_MODE, string> = Object.freeze({
    [THEME_MODE.SYSTEM]: "System",
    [THEME_MODE.DARK]: "Dark",
    [THEME_MODE.LIGHT]: "Light",
});

/**
 * Displays a theme toggle menu item.
 *
 * @return
 */
const ThemeToggleMenuItem = () => {
    const mode = useThemeStore((state) => state.mode);

    const handleClick = useCallback(() => {
        const {setMode} = useThemeStore.getState();
        if (mode === THEME_MODE.SYSTEM) {
            setMode(THEME_MODE.DARK);
        } else if (mode === THEME_MODE.DARK) {
            setMode(THEME_MODE.LIGHT);
        } else {
            setMode(THEME_MODE.SYSTEM);
        }
    }, [mode]);

    return (
        <Item
            icon={THEME_MODE_TO_ICON[mode]}
            onClick={handleClick}
        >
            {THEME_MODE_TO_LABEL[mode]}
        </Item>
    );
};


export default ThemeToggleMenuItem;
