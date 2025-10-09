interface BuildSearchQueryProps {
    selectItemList: string;
    databaseName: string;
    booleanExpression?: string | undefined;
    sortItemList?: string | undefined;
    limitValue?: string | undefined;
    startTimestamp: number;
    endTimestamp: number;
    timestampKey: string;
}

interface BuildTimelineQueryProps {
    databaseName: string;
    startTimestamp: number;
    endTimestamp: number;
    booleanExpression?: string | undefined;
    bucketCount: number;
    timestampKey: string;
}

/**
 * Default limit for presto search query
 */
const DEFAULT_SEARCH_LIMIT = 1000;

export type {
    BuildSearchQueryProps,
    BuildTimelineQueryProps,
};
export {DEFAULT_SEARCH_LIMIT};
