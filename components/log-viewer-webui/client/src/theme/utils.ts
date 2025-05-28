import {theme} from "antd";

import {THEME_MODE} from "./typings";


/**
 * Detects Antd theme algorithm from system theme.
 *
 * @return The system theme algorithm.
 */
const getSystemAlgorithm = () => {
    return window.matchMedia("(prefers-color-scheme: dark)").matches ?
        theme.darkAlgorithm :
        theme.defaultAlgorithm;
};

/**
 * Detects Antd theme algorithm for the given theme mode.
 *
 * @param mode
 * @return The theme algorithm.
 */
const getAlgorithm = (mode: THEME_MODE) => {
    if (mode === THEME_MODE.SYSTEM) {
        return getSystemAlgorithm();
    } else if (mode === THEME_MODE.DARK) {
        return theme.darkAlgorithm;
    }

    return theme.defaultAlgorithm;
};


export {
    getAlgorithm,
    getSystemAlgorithm,
};
