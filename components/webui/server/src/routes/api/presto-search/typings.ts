import settings from "../../../../settings.json" with {type: "json"};


/**
 * Maximum number of Presto search results to store in MongoDB.
 */
export const MAX_PRESTO_SEARCH_RESULTS: number = settings.MaxSearchResults;
