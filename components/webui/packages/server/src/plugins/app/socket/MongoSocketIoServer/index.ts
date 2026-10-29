/* eslint-disable max-lines */
/* eslint-disable no-warning-comments */
// TODO: Move listeners to a separate file to reduce lines
// Reference: https://github.com/socketio/socket.io/blob/main/examples/basic-crud-application/server/lib/todo-management/todo.handlers.ts

import {lookup as dnsLookup} from "node:dns/promises";

import fastifyHttpProxy from "@fastify/http-proxy";
import type {
    ClientToServerEvents,
    InterServerEvents,
    QueryId,
    Response,
    ServerToClientEvents,
    SocketData,
} from "@webui/common/socket";
import {
    FastifyBaseLogger,
    FastifyInstance,
} from "fastify";
import fastifyPlugin from "fastify-plugin";
import {Db} from "mongodb";
import {Server} from "socket.io";

import MongoWatcherCollection from "./MongoWatcherCollection.js";
import {
    ConnectionId,
    MongoCustomSocket,
    QueryParameters,
} from "./typings.js";
import {
    getQuery,
    getQueryHash,
    removeItemFromArray,
} from "./utils.js";


/**
 * Integrates MongoDB with Socket.IO to provide real-time updates for MongoDB queries.
 *
 * TODO: In current implementation, multiple queries in the same collection send updates over
 * one socket with the same event name. A potential improvement would be to use different event
 * names per query, limiting the number of events listeners triggered in the client.
 */
class MongoSocketIoServer {
    #logger: FastifyBaseLogger;

    #io: Server<ClientToServerEvents, ServerToClientEvents, InterServerEvents, SocketData>;

    // Collections with active queries.
    #collections: Map<string, MongoWatcherCollection> = new Map();

    // Mapping of active queries to their hashes.
    #queryIdToQueryHashMap: Map<QueryId, string> = new Map();

    // Mapping of connection IDs to the query IDs they are subscribed to. A connection can
    // subscribe to the same queryID multiple times, so the list can contain duplicates.
    #subscribedQueryIdsMap: Map<ConnectionId, QueryId[]> = new Map();

    // Counter for generating unique query IDs.
    #queryIdCounter: QueryId = 0;

    readonly #mongoDb: Db;

    /**
     * @param io
     * @param logger
     * @param mongoDb
     */
    private constructor (
        io: Server<ClientToServerEvents, ServerToClientEvents, InterServerEvents, SocketData>,
        logger: FastifyBaseLogger,
        mongoDb: Db
    ) {
        this.#io = io;
        this.#logger = logger;
        this.#mongoDb = mongoDb;
        this.#registerEventListeners();
    }

    /**
     * Creates a new MongoSocketIoServer.
     *
     * @param fastify
     * @return
     * @throws {Error} When MongoDB database not found
     */
    static async create (
        fastify: FastifyInstance
    ): Promise<MongoSocketIoServer> {
        const mongoDb = fastify.mongo.db;

        if ("undefined" === typeof mongoDb) {
            throw new Error("MongoDB database not found");
        }

        // Fastify listens on all resolved addresses for localhost (e.g. `::1` and `127.0.0.1`), but
        // socket.io can only intercept requests on the main server which listens only on the
        // primary address. When multiple addresses are resolved, we proxy socket.io requests
        // from secondary addresses to the main server.
        try {
            const addresses = await dnsLookup(fastify.config.HOST, {all: true});
            if (1 < addresses.length) {
                fastify.log.warn(
                    `Multiple addresses found for ${fastify.config.HOST}: ${
                        JSON.stringify(addresses)}. ` +
                    "Proxying all socket.io requests to the main server."
                );

                const protocol = fastify.initialConfig.https ?
                    "https" :
                    "http";

                fastify.register(fastifyHttpProxy, {
                    upstream: `${protocol}://${fastify.config.HOST}:${fastify.config.PORT}/socket.io/`,
                    prefix: "/socket.io/",
                });
            }
        } catch (e) {
            fastify.log.error(`DNS lookup failed: ${e instanceof Error ?
                e.message :
                JSON.stringify(e)}`);
        }

        const io = new Server<
            ClientToServerEvents,
            ServerToClientEvents,
            InterServerEvents,
            SocketData
        >(fastify.server);

        return new MongoSocketIoServer(io, fastify.log, mongoDb);
    }

    /**
     * Registers event listeners on socket connection.
     */
    #registerEventListeners () {
        this.#io.on("connection", (socket) => {
            this.#logger.info(`New socket connected with ID:${socket.id}`);
            socket.on("disconnect", this.#disconnectListener.bind(this, socket));
            socket.on(
                "collection::find::subscribe",
                this.#collectionFindSubscribeListener.bind(this, socket)
            );
            socket.on(
                "collection::find::unsubscribe",
                this.#collectionFindUnsubscribeListener.bind(this, socket)
            );
        });
    }

    /**
     * Listener for socket disconnection events.
     *
     * @param socket
     */
    async #disconnectListener (socket: MongoCustomSocket) {
        this.#logger.info(`Socket:${socket.id} disconnected`);
        const subscribedQueryIds = this.#subscribedQueryIdsMap.get(socket.id);

        if ("undefined" === typeof subscribedQueryIds) {
            return;
        }

        for (const queryId of subscribedQueryIds) {
            this.#unsubscribe(socket, queryId);
        }

        this.#subscribedQueryIdsMap.delete(socket.id);
        this.#logger.debug(
            "Subscribed queryIDs map" +
            ` ${JSON.stringify(Array.from(this.#subscribedQueryIdsMap.entries()))}`
        );
    }

    /**
     * Checks if a collection exists in the MongoDB database.
     *
     * @param collectionName
     * @return Whether the collection exists.
     */
    async #hasCollection (collectionName: string): Promise<boolean> {
        const collections = await this.#mongoDb.listCollections().toArray();
        return collections.some((collection) => collection.name === collectionName);
    }

    /**
     * Adds the query ID to the connection's subscribed query IDs.
     *
     * @param queryId
     * @param socketId
     */
    #addQueryIdToSubscribedList (queryId: QueryId, socketId: ConnectionId) {
        const subscribedQueryIds = this.#subscribedQueryIdsMap.get(socketId);
        if ("undefined" === typeof subscribedQueryIds) {
            this.#subscribedQueryIdsMap.set(socketId, [queryId]);

            return;
        }

        this.#subscribedQueryIdsMap.set(socketId, [...subscribedQueryIds,
            queryId]);
    }

    /**
     * Gets query ID based on query parameters. If not found, creates a new ID.
     *
     * @param queryParams
     * @return the query ID.
     */
    #getQueryId (queryParams: QueryParameters): number {
        const queryHash = getQueryHash(queryParams);
        for (const [queryId, hash] of this.#queryIdToQueryHashMap.entries()) {
            if (hash === queryHash) {
                return queryId;
            }
        }
        const queryId = this.#queryIdCounter;
        this.#queryIdToQueryHashMap.set(queryId, queryHash);

        // JS is single threaded and ++ is atomic, so we can safely increment the global counter.
        this.#queryIdCounter++;

        return queryId;
    }

    /**
     * Gets an existing watcher collection or creates a new one if it doesn't exist.
     *
     * @param collectionName
     * @return The watcher collection instance.
     */
    #getOrCreateWatcherCollection (
        collectionName: string
    ): MongoWatcherCollection {
        let watcherCollection = this.#collections.get(collectionName);
        if ("undefined" === typeof watcherCollection) {
            watcherCollection = new MongoWatcherCollection(
                collectionName,
                this.#logger,
                this.#mongoDb
            );
            this.#logger.debug(`Initialize Mongo watcher collection:${collectionName}.`);
            this.#collections.set(collectionName, watcherCollection);
        }

        return watcherCollection;
    }

    /**
     * Listener for subscribing to a find query. The client will receive updates whenever
     * the query results change.
     *
     * @param socket
     * @param requestArgs
     * @param requestArgs.query
     * @param requestArgs.options
     * @param requestArgs.collectionName
     * @param callback
     */
    async #collectionFindSubscribeListener (
        socket: MongoCustomSocket,
        requestArgs: {collectionName: string; query: object; options: object},
        callback: (res: Response<{queryId: number; initialDocuments: object[]}>) => void
    ): Promise<void> {
        const {collectionName, query, options} = requestArgs;

        this.#logger.debug(
            `Socket:${socket.id} requested query:${JSON.stringify(query)} ` +
            `with options:${JSON.stringify(options)} to collection:${collectionName}`
        );

        const hasCollection = await this.#hasCollection(collectionName);
        if (false === hasCollection) {
            this.#logger.error(`Collection ${collectionName} does not exist in MongoDB`);
            callback({
                error: `Collection ${collectionName} does not exist in MongoDB on server`,
            });

            return;
        }

        const watcherCollection = this.#getOrCreateWatcherCollection(collectionName);

        const queryParameters: QueryParameters = {collectionName, query, options};
        const queryId = this.#getQueryId(queryParameters);

        await this.#subscribeToQuery(watcherCollection, queryParameters, queryId, socket);

        const initialDocuments = await watcherCollection.find(queryParameters);
        callback({data: {queryId, initialDocuments}});

        this.#addQueryIdToSubscribedList(queryId, socket.id);
        this.#logger.info(
            `Socket:${socket.id} subscribed to query:${JSON.stringify(query)} ` +
            `with options:${JSON.stringify(options)} ` +
            `on collection:${collectionName} with ID:${queryId}`
        );
    }

    /**
     * Subscribes to query updates.
     *
     * @param watcherCollection
     * @param queryParameters
     * @param queryId
     * @param socket
     */
    async #subscribeToQuery (
        watcherCollection: MongoWatcherCollection,
        queryParameters: QueryParameters,
        queryId: QueryId,
        socket: MongoCustomSocket
    ): Promise<void> {
        if (false === watcherCollection.hasWatcher(queryId)) {
            const emitUpdate = (data: object[]) => {
                this.#io.to(`${queryId}`).emit("collection::find::update", {queryId, data});
            };

            watcherCollection.createWatcher(queryParameters, queryId, emitUpdate);
        }
        await watcherCollection.subscribe(queryId, socket);
    }

    /**
     * Unsubscribes from a query.
     *
     * @param socket
     * @param queryId
     */
    #unsubscribe (socket: MongoCustomSocket, queryId: number) {
        const queryHash: string | undefined = this.#queryIdToQueryHashMap.get(queryId);
        if ("undefined" === typeof queryHash) {
            this.#logger.error(`Query:${queryId} not found in query map`);

            return;
        }

        const queryParams: QueryParameters = getQuery(queryHash);

        const collection = this.#collections.get(queryParams.collectionName);
        if ("undefined" === typeof collection) {
            this.#logger.error(`${queryParams.collectionName} is missing from server`);

            return;
        }

        const isLastSubscriber = collection.unsubscribe(queryId, socket.id);
        this.#logger.info(`Socket:${socket.id} unsubscribed from query:${queryId}`);

        if (isLastSubscriber) {
            this.#logger.debug(`Query:${queryId} deleted from query map.`);
            this.#queryIdToQueryHashMap.delete(queryId);
        }

        this.#logger.debug(
            "Query ID to query hash map:" +
            ` ${JSON.stringify(Array.from(this.#queryIdToQueryHashMap.entries()))}`
        );

        if (false === collection.isReferenced()) {
            this.#logger.debug(`Collection:${queryParams.collectionName}` +
            " deallocated from server.");
            this.#collections.delete(queryParams.collectionName);
        }
    }

    /**
     * Listener for unsubscribing from a find query.
     *
     * @param socket
     * @param requestArgs
     * @param requestArgs.queryId
     */
    async #collectionFindUnsubscribeListener (
        socket: MongoCustomSocket,
        requestArgs: {queryId: number}
    ): Promise<void> {
        const {queryId} = requestArgs;
        this.#logger.debug(
            `Socket:${socket.id} requested unsubscription to query:${queryId}`
        );

        const subscribedQueryIds = this.#subscribedQueryIdsMap.get(socket.id);
        if ("undefined" === typeof subscribedQueryIds ||
            false === subscribedQueryIds.includes(queryId)
        ) {
            this.#logger.error(`Socket ${socket.id} is not subscribed to ${queryId}`);

            return;
        }

        this.#unsubscribe(socket, queryId);
        await socket.leave(queryId.toString());

        removeItemFromArray(subscribedQueryIds, queryId);

        this.#logger.debug(
            `Subscribed queryIDs map ${
                JSON.stringify(Array.from(this.#subscribedQueryIdsMap.entries()))}`
        );
    }
}

declare module "fastify" {
    export interface FastifyInstance {
        MongoSocketIoServer: MongoSocketIoServer;
    }
}

export default fastifyPlugin(
    async (fastify) => {
        fastify.decorate("MongoSocketIoServer", await MongoSocketIoServer.create(fastify));
    },
    {
        name: "MongoSocketIoServer",
    }
);
