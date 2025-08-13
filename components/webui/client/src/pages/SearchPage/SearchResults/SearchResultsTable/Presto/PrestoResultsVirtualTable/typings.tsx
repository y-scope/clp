/**
 * Structure of dynamic Presto search results data.
 */
interface PrestoSearchResult {
    _id: string;
    row: Record<string, unknown>;
}

export type {PrestoSearchResult};
export {getPrestoSearchResultsTableColumns} from "./utils";
