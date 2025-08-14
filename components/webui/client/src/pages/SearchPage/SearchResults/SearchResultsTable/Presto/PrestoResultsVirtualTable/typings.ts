import type {PrestoRowObject} from "../../../../../../../../common/index.js";


/**
 * Structure of dynamic Presto search results data.
 */
interface PrestoSearchResult extends PrestoRowObject {
    _id: string;
}

export type {PrestoSearchResult};
