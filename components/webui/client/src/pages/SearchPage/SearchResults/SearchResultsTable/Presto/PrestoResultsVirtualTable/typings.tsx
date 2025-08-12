import {PRESTO_DATA_PROPERTY} from "../../../../../../../../common";


/**
 * Structure of dynamic Presto search results data.
 */
interface PrestoSearchResult {
    _id: string;
    [PRESTO_DATA_PROPERTY]: Record<string, unknown>;
}

export type {PrestoSearchResult};
export {getPrestoSearchResultsTableColumns} from "./utils";
