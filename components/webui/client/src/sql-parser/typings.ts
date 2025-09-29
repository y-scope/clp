interface BuildSearchQueryProps {
    selectItemList: string;
    databaseName: string;
    booleanExpression: string;
    sortItemList: string;
    limitValue: number;
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

export type {
    BuildSearchQueryProps,
    BuildTimelineQueryProps,
};
