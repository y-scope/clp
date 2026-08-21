/**
 * Matching the `QueryJobType` class in `job_orchestration.query_scheduler.constants`.
 */
enum QUERY_JOB_TYPE {
    SEARCH_OR_AGGREGATION = 0,
    EXTRACT_IR,
    EXTRACT_JSON,
}

export {QUERY_JOB_TYPE};
