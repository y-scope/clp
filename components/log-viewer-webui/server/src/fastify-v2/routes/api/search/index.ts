import { SEARCH_MAX_NUM_RESULTS } from './constants.js';
import {SEARCH_SIGNAL} from "../../../plugins/app/search/SearchResultsMetadataCollection/typings.js";
import { updateSearchResultsMeta, updateSearchSignalWhenJobsFinish, createMongoIndexes } from './utils.js';
import {
    FastifyPluginAsyncTypebox,
    Type
  } from '@fastify/type-provider-typebox'
import { CreateSearchJobSchema, SearchJobSchema } from '../../../schemas/search.js'
import { FastifyErrorSchema } from '../../../schemas/error.js'
import { QuerySubmitError, ClearResultsError, QueryCancelError } from './errors.js';

  const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const {
      QueryJobsDbManager,
      SearchJobCollectionsManager,
      SearchResultsMetadataCollection,
    } = fastify;

     /**
     * Submits a search query and initiates the search process.
     */
    fastify.post(
        '/search',
        {
          schema: {
            body: CreateSearchJobSchema,
            response: {
              201: SearchJobSchema,
              500: FastifyErrorSchema,
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

            request.log.info({ args }, "/search args");

            let searchJobId: number;
            let aggregationJobId: number;

            try {
              searchJobId = await QueryJobsDbManager.submitSearchJob(args);
              aggregationJobId = await QueryJobsDbManager.submitAggregationJob(
                args,
                timeRangeBucketSizeMillis
              );
            } catch (err: unknown) {
              request.log.error( err , 'Unable to submit search/aggregation job to the SQL database');
              reply.code(500)
              return QuerySubmitError();
            }

            SearchResultsMetadataCollection.insertOne({
              _id: searchJobId.toString(),
              lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
              errorMsg: null
            });

            // Defer signal update until after response is sent
            setImmediate( async () => {
                await updateSearchSignalWhenJobsFinish({
                searchJobId,
                aggregationJobId,
                queryJobsDbManager: QueryJobsDbManager,
                searchJobCollectionsManager: SearchJobCollectionsManager,
                SearchResultsMetadataCollection,
                logger: request.log,
                });
            });

            await createMongoIndexes({
              searchJobId,
              searchJobCollectionsManager: SearchJobCollectionsManager,
              logger: request.log,
            });

            reply.code(201)

            return { searchJobId, aggregationJobId };
          }
    );

    /**
     * Clears the results of a search operation identified by jobId.
     */
    fastify.delete(
      '/search/results',
      {
        schema: {
          body: SearchJobSchema,
          response: {
            204: Type.Null(),
            500: FastifyErrorSchema,
          },
          tags: ['Search'],
        },
      },
      async function (request, reply) {
        const { searchJobId, aggregationJobId } = request.body;

        request.log.info({
          searchJobId,
          aggregationJobId,
        }, "/search/results args");

        try {
          await SearchJobCollectionsManager.dropCollection(searchJobId);
          await SearchJobCollectionsManager.dropCollection(aggregationJobId);
        } catch (err: unknown) {
          request.log.error(
            {
              err,
              searchJobId,
              aggregationJobId,
            },
            'Failed to clear search results'
          );

          reply.code(500);
          return ClearResultsError(searchJobId, aggregationJobId);
        }

        reply.code(204);
        return;
      }
    );


    fastify.post(
      '/search/cancel',
      {
        schema: {
          body: SearchJobSchema,
          response: {
            204: Type.Null(),
            500: FastifyErrorSchema,
          },
          tags: ['Search'],
        },
      },
      async function (request, reply) {
        const { searchJobId, aggregationJobId } = request.body;

        request.log.info(
          `/search/cancel searchJobId=${searchJobId}, aggregationJobId=${aggregationJobId}`
        );

        try {
          await QueryJobsDbManager.submitQueryCancellation(searchJobId);
          await QueryJobsDbManager.submitQueryCancellation(aggregationJobId);

          updateSearchResultsMeta({
            jobId: searchJobId,
            lastSignal: SEARCH_SIGNAL.RESP_QUERYING,
            SearchResultsMetadataCollection,
            logger: request.log,
            fields: {
              lastSignal: SEARCH_SIGNAL.RESP_DONE,
              errorMsg: 'Query cancelled before it could be completed.',
            },
          });

        } catch (err: unknown) {
          request.log.error(
            {
              err,
              searchJobId,
              aggregationJobId,
            },
            'Failed to submit cancel request'
          );
          reply.code(500);
          return QueryCancelError(searchJobId, aggregationJobId);
        }

        reply.code(204);
        return;
      }
    );
  }

  export default plugin