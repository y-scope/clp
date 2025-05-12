import {
    SEARCH_MAX_NUM_RESULTS,
    SEARCH_SIGNAL,
} from "../constants";

import { Collection } from 'mongodb'; // Assuming MongoDB is used for SearchResultsMetadataCollection

type UpdateSearchResultsMetaParams = {
    jobId: number;
    lastSignal: string;
    SearchResultsMetadataCollection: Collection;
};

type SearchResultsMetadataFields = {
    lastSignal?: string;
    errorMsg?: string | null;
    numTotalResults?: number;
};

/**
 * Modifies the search results metadata for a given job ID.
 *
 * @param {UpdateSearchResultsMetaParams} params
 * @param {SearchResultsMetadataFields} fields The fields to be updated in the search results metadata.
 */
const updateSearchResultsMeta = (
    {
        jobId,
        lastSignal,
        SearchResultsMetadataCollection,
    }: UpdateSearchResultsMetaParams,
    fields: SearchResultsMetadataFields
) => {
    const filter = {
        _id: jobId.toString(),
        lastSignal: lastSignal,
    };

    const modifier = {
        $set: fields,
    };

    logger.debug("SearchResultsMetadataCollection modifier = ", modifier);
    SearchResultsMetadataCollection.update(filter, modifier);
};

/**
 * Updates the search signal when the specified job finishes.
 *
 * @param {object} params
 * @param {number} params.searchJobId of the job to monitor
 * @param {number} params.aggregationJobId of the job to monitor
 * @param {QueryJobsDbManager} queryJobsDbManager
 * @param {SearchJobCollectionsManager} searchJobCollectionsManager
 * @param {SearchResultsMetadataCollection} SearchResultsMetadataCollection
 */
const updateSearchSignalWhenJobsFinish = async ({
    searchJobId,
    aggregationJobId,
    queryJobsDbManager,
    searchJobCollectionsManager,
    SearchResultsMetadataCollection,
}) => {
    let errorMsg;
    try {
        await queryJobsDbManager.awaitJobCompletion(searchJobId);
        await queryJobsDbManager.awaitJobCompletion(aggregationJobId);
    } catch (e) {
        errorMsg = e.message;
    }

    let numResultsInCollection = -1;
    try {
        numResultsInCollection = await searchJobCollectionsManager
            .getOrCreateCollection(searchJobId)
            .countDocuments();
    } catch (e) {
        if (ERROR_NAME_COLLECTION_DROPPED === e.error) {
            logger.warn(`Collection ${searchJobId} has been dropped.`);

            return;
        }
        throw e;
    }

    updateSearchResultsMeta({
        jobId: searchJobId,
        lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
        SearchResultsMetadataCollection,
    }, {
        lastSignal: SEARCH_SIGNAL.RESP_DONE,
        errorMsg: errorMsg,
        numTotalResults: Math.min(
            numResultsInCollection,
            SEARCH_MAX_NUM_RESULTS
        ),
    });
};

/**
 * Creates MongoDB indexes for a specific job's collection.
 *
 * @param {number} searchJobId used to identify the Mongo Collection to add indexes
 * @param {SearchJobCollectionsManager} searchJobCollectionsManager
 */
const createMongoIndexes = async (searchJobId, searchJobCollectionsManager) => {
    const timestampAscendingIndex = {
        key: {
            timestamp: 1,
            _id: 1,
        },
        name: "timestamp-ascending",
    };
    const timestampDescendingIndex = {
        key: {
            timestamp: -1,
            _id: -1,
        },
        name: "timestamp-descending",
    };

    const queryJobCollection = searchJobCollectionsManager.getOrCreateCollection(searchJobId);
    const queryJobRawCollection = queryJobCollection.rawCollection();
    await queryJobRawCollection.createIndexes([
        timestampAscendingIndex,
        timestampDescendingIndex,
    ]);
};

import {
    FastifyPluginAsyncTypebox,
    Type
  } from '@fastify/type-provider-typebox'
  //import { CredentialsSchema } from '../../../schemas/auth.js'
   import { CreateSearchJobSchema, SearchJobResultSchema } from '../../../schemas/search.js'

  const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const { QueryJobsDbManager,
      SearchJobCollectionsManager,
      SearchResultsMetadataCollection
    } = fastify

    fastify.post(
        '/search',
        {
          schema: {
            body: CreateSearchJobSchema,
            response: {
              200: SearchJobResultSchema,
              500: Type.Object({ message: Type.String() }),
            },
            tags: ['Search']
          },
        },
        async function (request, reply) {
            const {
              queryString,
              timestampBegin,
              timestampEnd,
              ignoreCase,
              timeRangeBucketSizeMillis
            } = request.body;

            const args = {
              query_string: queryString,
              begin_timestamp: timestampBegin,
              end_timestamp: timestampEnd,
              ignore_case: ignoreCase,
              max_num_results: SEARCH_MAX_NUM_RESULTS
            };

            request.log.info({ args }, 'Submitting search query');

            let searchJobId: number;
            let aggregationJobId: number;

            try {
              searchJobId = await QueryJobsDbManager.submitSearchJob(args);
              aggregationJobId = await QueryJobsDbManager.submitAggregationJob(
                args,
                timeRangeBucketSizeMillis
              );
            } catch (err: any) {
              const message = 'Unable to submit search/aggregation job to the SQL database';
              request.log.error(err, message);
              return reply.internalServerError(message);
            }

            // I need to update the schema.
            SearchResultsMetadataCollection.insertOne({
              _id: searchJobId.toString(),
              lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
              errorMsg: null
            });

            // Defer signal update until after response is sent
            setImmediate(() => {
              updateSearchSignalWhenJobsFinish({
                searchJobId,
                aggregationJobId,
                queryJobsDbManager: QueryJobsDbManager,
                searchJobCollectionsManager: SearchJobCollectionsManager,
                SearchResultsMetadataCollection,
              }).catch(err =>
                request.log.error(err, 'Deferred update failed')
              );
            });

            await createMongoIndexes(searchJobId, SearchJobCollectionsManager);

            return { searchJobId, aggregationJobId };
          }
    );

    fastify.delete(
      '/search/results',
      {
        schema: {
          body: SearchJobResultSchema,
          response: {
            204: Type.Null,
            500: Type.Object({ message: Type.String() }),
          },
          tags: ['Search'],
        },
      },
      async function (request, reply) {
        const { searchJobId, aggregationJobId } = request.body;

        request.log.info(
          `search.clearResults searchJobId=${searchJobId}, aggregationJobId=${aggregationJobId}`
        );

        try {
          await SearchJobCollectionsManager.dropCollection(searchJobId);
          await SearchJobCollectionsManager.dropCollection(aggregationJobId);

          reply.code(204);
        } catch (err: any) {
          const message = `Failed to clear search results for searchJobId=${searchJobId}, aggregationJobId=${aggregationJobId}`;
          request.log.error(err, message);
          return reply.internalServerError(message);
        }
      }
    );


    fastify.post(
      '/search/cancel',
      {
        schema: {
          body: SearchJobResultSchema,
          response: {
            204: Type.Null,
            500: Type.Object({ message: Type.String()}),
          },
          tags: ['Search'],
        },
      },
      async function (request, reply) {
        const { searchJobId, aggregationJobId } = request.body;

        request.log.info(
          `search.cancelOperation searchJobId=${searchJobId}, aggregationJobId=${aggregationJobId}`
        );

        try {
          await QueryJobsDbManager.submitQueryCancellation(searchJobId);
          await QueryJobsDbManager.submitQueryCancellation(aggregationJobId);

          updateSearchResultsMeta(
            {
              jobId: searchJobId,
              lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
              SearchResultsMetadataCollection,
            },
            {
              lastSignal: SEARCH_SIGNAL.RESP_DONE,
              errorMsg: 'Query cancelled before it could be completed.',
            }
          );

          reply.code(204);
        } catch (err: any) {
          const message = `Failed to submit cancel request for searchJobId=${searchJobId}, aggregationJobId=${aggregationJobId}`;
          request.log.error(err, message);
          return reply.internalServerError(message);
        }
      }
    );
  }

  export default plugin