import { SEARCH_SIGNAL, SEARCH_MAX_NUM_RESULTS } from '../../../plugins/app/search/constants.js';

import { updateSearchResultsMeta, updateSearchSignalWhenJobsFinish, createMongoIndexes } from './utils.js';
import {
    FastifyPluginAsyncTypebox,
    Type
  } from '@fastify/type-provider-typebox'
import { CreateSearchJobSchema, SearchJobResultSchema } from '../../../schemas/search.js'

  const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const { QueryJobsDbManager,
      SearchJobCollectionsManager,
      SearchResultsMetadataCollection,
      log: logger
    } = fastify;

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
            } catch (err: unknown) {
              const errorMsg = 'Unable to submit search/aggregation job to the SQL database';
              request.log.error(errorMsg, err);
              return reply.internalServerError(errorMsg);
            }

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
                logger: request.log,
                }).catch(err =>
                request.log.error(err, 'Deferred update failed')
                );
            });

            await createMongoIndexes({
              searchJobId,
              searchJobCollectionsManager: SearchJobCollectionsManager,
              logger,
            });

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

          updateSearchResultsMeta({
            jobId: searchJobId,
            lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
            SearchResultsMetadataCollection,
            logger,
            fields: {
              lastSignal: SEARCH_SIGNAL.RESP_DONE,
              errorMsg: 'Query cancelled before it could be completed.',
            },
          });

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