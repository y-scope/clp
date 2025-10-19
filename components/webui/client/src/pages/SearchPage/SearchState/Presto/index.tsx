import {Nullable} from "@webui/common/utility-types";
import {create} from "zustand";

import {PRESTO_SQL_INTERFACE} from "./typings";


/**
 * Default values of the Presto search state.
 */
const PRESTO_SEARCH_STATE_DEFAULT = Object.freeze({
    from: null,
    orderBy: "",
    select: "*",
    sqlInterface: PRESTO_SQL_INTERFACE.FREEFORM,
    timestampKey: null,
    where: "",
});

interface PrestoSearchState {
    /**
     * FROM input.
     */
    from: Nullable<string>;

    /**
     * ORDER BY input.
     */
    orderBy: string;

    /**
     * SELECT input.
     */
    select: string;

    /**
     * Presto SQL interface.
     */
    sqlInterface: PRESTO_SQL_INTERFACE;

    /**
     * Selected timestamp key column.
     */
    timestampKey: Nullable<string>;

    /**
     * WHERE input.
     */
    where: string;

    setSqlInterface: (iface: PRESTO_SQL_INTERFACE) => void;
    updateFrom: (database: Nullable<string>) => void;
    updateOrderBy: (items: string) => void;
    updateSelect: (items: string) => void;
    updateTimestampKey: (key: Nullable<string>) => void;
    updateWhere: (expression: string) => void;
}

const usePrestoSearchState = create<PrestoSearchState>((set) => ({
    ...PRESTO_SEARCH_STATE_DEFAULT,
    setSqlInterface: (iface) => {
        set({sqlInterface: iface});
    },
    updateFrom: (database) => {
        set({from: database});
    },
    updateOrderBy: (items) => {
        set({orderBy: items});
    },
    updateSelect: (items) => {
        set({select: items});
    },
    updateTimestampKey: (key) => {
        set({timestampKey: key});
    },
    updateWhere: (expression) => {
        set({where: expression});
    },
}));

export {PRESTO_SEARCH_STATE_DEFAULT};
export default usePrestoSearchState;
