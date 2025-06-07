import type {MappingAlgorithm} from "antd/es/theme/interface";
import {create} from "zustand";
import {persist} from "zustand/middleware";

import {THEME_MODE} from "./typings";
import {
    getAlgorithm,
    getSystemAlgorithm,
} from "./utils";


interface ThemeValues {
    algorithm: MappingAlgorithm;
    mode: THEME_MODE;
}

interface ThemeActions {
    setMode: (mode: THEME_MODE) => void;
}

type ThemeState = ThemeValues & ThemeActions;

const THEME_DEFAULT: ThemeValues = {
    algorithm: getSystemAlgorithm(),
    mode: THEME_MODE.SYSTEM,
};

const useThemeStore = create(
    persist<ThemeState>(
        (set) => ({
            ...THEME_DEFAULT,
            setMode: (newMode: THEME_MODE) => {
                set({mode: newMode});
                set({algorithm: getAlgorithm(newMode)});
            },
        }),
        {
            name: "theme-storage",
            merge: (persistedState, currentState) => {
                const safePersistedState = persistedState as Partial<ThemeState>;

                return {
                    ...currentState,
                    ...safePersistedState,
                    ...(
                        safePersistedState.mode &&
                        {algorithm: getAlgorithm(safePersistedState.mode)}
                    ),
                };
            },
        }
    )
);


export default useThemeStore;
