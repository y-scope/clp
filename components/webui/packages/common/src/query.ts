/**
 * Matching the `QueryJobType` class in `job_orchestration.query_scheduler.constants`.
 */
enum QUERY_JOB_TYPE {
    SEARCH_OR_AGGREGATION = 0,
    EXTRACT_IR,
    EXTRACT_JSON,
}

/**
 * List of valid extract job types.
 */
const EXTRACT_JOB_TYPES = new Set([
    QUERY_JOB_TYPE.EXTRACT_IR,
    QUERY_JOB_TYPE.EXTRACT_JSON,
]);

export {
    EXTRACT_JOB_TYPES,
    QUERY_JOB_TYPE,
};
