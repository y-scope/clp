import {create} from "zustand";
import {PRESTO_SQL_INTERFACE} from "./typings";


/**
 * Default values of the Presto search state.
 */
const PRESTO_SEARCH_STATE_DEFAULT = Object.freeze({
    sqlInterface: PRESTO_SQL_INTERFACE.FREEFORM,
});

interface PrestoSearchState {
    /**
     * Presto SQL interface.
     */
    sqlInterface: PRESTO_SQL_INTERFACE;

    setSqlInterface: (iface: PRESTO_SQL_INTERFACE) => void;
}

const usePrestoSearchState = create<PrestoSearchState>((set) => ({
    ...PRESTO_SEARCH_STATE_DEFAULT,
    setSqlInterface: (iface) => {
        set({sqlInterface: iface});
    },
}));

export {PRESTO_SEARCH_STATE_DEFAULT};
export default usePrestoSearchState;
