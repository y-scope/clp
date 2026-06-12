import {create} from "zustand";
import {message} from "antd";
import type {PrestoSearchResult} from "@webui/common/presto";
import {Nullable} from "@webui/common/utility-types";

import {downloadTextFile} from "../../../../utils/download";
import {formatExportFilenameTimestamp} from "../../SearchResults/SearchResultsTable/Native/utils";
import {formatPrestoResultAsJsonl} from "../../SearchResults/SearchResultsTable/Presto/PrestoResultsVirtualTable/utils";
import {PRESTO_SQL_INTERFACE} from "./typings";


/**
 * Default values of the Presto search state.
 */
const PRESTO_SEARCH_STATE_DEFAULT = Object.freeze({
    cachedGuidedSearchQueryString: "",
    errorMsg: null,
    errorName: null,
    orderBy: "",
    prestoSearchResults: null as Nullable<PrestoSearchResult[]>,
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
     * Exports all Presto search results as a JSONL file download.
     */
    handlePrestoSearchResultsExport: () => void;

    /**
     * ORDER BY input.
     */
    orderBy: string;

    /**
     * Current Presto search results for export.
     */
    prestoSearchResults: Nullable<PrestoSearchResult[]>;

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
    updatePrestoSearchResults: (results: Nullable<PrestoSearchResult[]>) => void;
    updateQueryDrawerOpen: (open: boolean) => void;
    updateSelect: (items: string) => void;
    updateTimestampKey: (key: Nullable<string>) => void;
    updateWhere: (expression: string) => void;
}

const usePrestoSearchState = create<PrestoSearchState>((set, get) => ({
    ...PRESTO_SEARCH_STATE_DEFAULT,
    handlePrestoSearchResultsExport: () => {
        const {prestoSearchResults} = get();
        if (null === prestoSearchResults || 0 === prestoSearchResults.length) {
            return;
        }

        try {
            downloadTextFile(
                prestoSearchResults.map(
                    (r) => `${formatPrestoResultAsJsonl(r)}\n`
                ),
                `presto-search-results-${formatExportFilenameTimestamp()}.jsonl`
            );
            message.success(`Exported ${prestoSearchResults.length} results`);
        } catch (e) {
            message.error("Failed to export results");
            console.error(e);
        }
    },
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
    updatePrestoSearchResults: (results) => {
        set({prestoSearchResults: results});
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
