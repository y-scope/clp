import {Nullable} from "@webui/common/utility-types";
import {create} from "zustand";

import {PRESTO_SQL_INTERFACE} from "./typings";


/**
 * Default values of the Presto search state.
 */
const PRESTO_SEARCH_STATE_DEFAULT = Object.freeze({
    sqlInterface: PRESTO_SQL_INTERFACE.FREEFORM,
    timestampKey: null,
    select: "*",
    from: null,
    where: "",
    orderBy: "",
    limit: 10,
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

    /**
     * SELECT input.
     */
    select: string;

    /**
     * FROM input.
     */
    from: Nullable<string>;

    /**
     * WHERE input.
     */
    where: string;

    /**
     * ORDER BY input.
     */
    orderBy: string;

    /**
     * LIMIT input.
     */
    limit: number;

    setSqlInterface: (iface: PRESTO_SQL_INTERFACE) => void;
    updateTimestampKey: (key: Nullable<string>) => void;
    updateSelect: (items: string) => void;
    updateFrom: (database: Nullable<string>) => void;
    updateWhere: (expression: string) => void;
    updateOrderBy: (items: string) => void;
    updateLimit: (value: number) => void;
}

const usePrestoSearchState = create<PrestoSearchState>((set) => ({
    ...PRESTO_SEARCH_STATE_DEFAULT,
    setSqlInterface: (iface) => {
        set({sqlInterface: iface});
    },
    updateTimestampKey: (key) => {
        set({timestampKey: key});
    },
    updateSelect: (items) => {
        set({select: items});
    },
    updateFrom: (database) => {
        set({from: database});
    },
    updateWhere: (expression) => {
        set({where: expression});
    },
    updateOrderBy: (items) => {
        set({orderBy: items});
    },
    updateLimit: (value) => {
        set({limit: value});
    },
}));

export {PRESTO_SEARCH_STATE_DEFAULT};
export default usePrestoSearchState;
