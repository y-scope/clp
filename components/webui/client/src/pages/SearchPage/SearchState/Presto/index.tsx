import {Nullable} from "@webui/common/utility-types";
import {create} from "zustand";

import {PRESTO_SQL_INTERFACE} from "./typings";


/**
 * Default values of the Presto search state.
 */
const PRESTO_SEARCH_STATE_DEFAULT = Object.freeze({
    cachedGuidedSearchQueryString: "",
    errorMsg: null,
    errorName: null,
    orderBy: "",
    queryDrawerOpen: false,
    select: "*",
    sqlInterface: PRESTO_SQL_INTERFACE.GUIDED,
    timestampKey: null,
    where: "",
});

interface PrestoSearchState {
    /**
     * Last submitted guided search query string.
     */
    cachedGuidedSearchQueryString: string;

    /**
     * Presto error message if query failed.
     */
    errorMsg: Nullable<string>;

    /**
     * Presto error name if query failed.
     */
    errorName: Nullable<string>;

    /**
     * ORDER BY input.
     */
    orderBy: string;

    /**
     * Whether the query preview drawer is open.
     */
    queryDrawerOpen: boolean;

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
    updateCachedGuidedSearchQueryString: (query: string) => void;
    updateErrorMsg: (msg: Nullable<string>) => void;
    updateErrorName: (name: Nullable<string>) => void;
    updateOrderBy: (items: string) => void;
    updateQueryDrawerOpen: (open: boolean) => void;
    updateSelect: (items: string) => void;
    updateTimestampKey: (key: Nullable<string>) => void;
    updateWhere: (expression: string) => void;
}

const usePrestoSearchState = create<PrestoSearchState>((set) => ({
    ...PRESTO_SEARCH_STATE_DEFAULT,
    setSqlInterface: (iface) => {
        set({sqlInterface: iface});
    },
    updateCachedGuidedSearchQueryString: (query) => {
        set({cachedGuidedSearchQueryString: query});
    },
    updateErrorMsg: (msg) => {
        set({errorMsg: msg});
    },
    updateErrorName: (name) => {
        set({errorName: name});
    },
    updateOrderBy: (items) => {
        set({orderBy: items});
    },
    updateQueryDrawerOpen: (open) => {
        set({queryDrawerOpen: open});
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
