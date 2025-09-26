import {Nullable} from "@webui/common/utility-types";
import {create} from "zustand";

import {PRESTO_SQL_INTERFACE} from "./typings";


/**
 * Default values of the Presto search state.
 */
const PRESTO_SEARCH_STATE_DEFAULT = Object.freeze({
    sqlInterface: PRESTO_SQL_INTERFACE.FREEFORM,
    timestampKey: null,
});

interface PrestoSearchState {
    /**
     * Presto SQL interface.
     */
    sqlInterface: PRESTO_SQL_INTERFACE;

    /**
     * Selected timestamp key column.
     */
    timestampKey: Nullable<string>;

    setSqlInterface: (iface: PRESTO_SQL_INTERFACE) => void;
    updateTimestampKey: (key: Nullable<string>) => void;
}

const usePrestoSearchState = create<PrestoSearchState>((set) => ({
    ...PRESTO_SEARCH_STATE_DEFAULT,
    setSqlInterface: (iface) => {
        set({sqlInterface: iface});
    },
    updateTimestampKey: (key) => {
        set({timestampKey: key});
    },
}));

export {PRESTO_SEARCH_STATE_DEFAULT};
export default usePrestoSearchState;
