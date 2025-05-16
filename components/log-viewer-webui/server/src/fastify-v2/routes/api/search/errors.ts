import createError from '@fastify/error';

enum ErrorCodes {
  QuerySubmitError = 'FST_QUERY_SUBMIT_ERROR',
  ClearResultsError = 'FST_CLEAR_RESULTS_ERROR',
  QueryCancelError = 'FST_QUERY_CANCEL_ERROR',
}

const QuerySubmitError = createError(
    ErrorCodes.QuerySubmitError,
  'Unable to submit search/aggregation job to the SQL database',
);

const ClearResultsError = createError(
    ErrorCodes.ClearResultsError,
  'Failed to clear search results for searchJobId=%s, aggregationJobId=%s',
);

const QueryCancelError = createError(
    ErrorCodes.QueryCancelError,
  'Failed to submit cancel request for searchJobId=%s, aggregationJobId=%s',
);

export {QuerySubmitError, ClearResultsError, QueryCancelError };
