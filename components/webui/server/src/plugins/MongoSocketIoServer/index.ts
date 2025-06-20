/* eslint-disable max-lines */
/* eslint-disable no-warning-comments */
// TODO: Move listeners to a separate file to reduce lines
// Reference: https://github.com/socketio/socket.io/blob/main/examples/basic-crud-application/server/lib/todo-management/todo.handlers.ts

import {
    FastifyInstance,
    FastifyPluginAsync,
} from "fastify";
import fastifyPlugin from "fastify-plugin";
import {Db} from "mongodb";
import {Server} from "socket.io";

import type {
    ClientToServerEvents,
    InterServerEvents,
    QueryId,
    Response,
    ServerToClientEvents,
    SocketData,
} from "../../../../common/index.js";
import MongoWatcherCollection from "./MongoWatcherCollection.js";
import {
    ConnectionId,
    DbOptions,
    MongoCustomSocket,
    QueryParameters,
} from "./typings.js";
import {
    getQuery,
    getQueryHash,
    initializeMongoClient,
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
    #fastify: FastifyInstance;

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
     * Private constructor for MongoSocketIoServer. This is not intended to be invoked publicly.
     * Instead, use MongoSocketIoServer.create() to create a new instance of the class.
     *
     * @param fastify
     * @param mongoDb
     */
    constructor (fastify: FastifyInstance, mongoDb: Db) {
        this.#fastify = fastify;
        this.#mongoDb = mongoDb;
        this.#io = new Server<
            ClientToServerEvents,
            ServerToClientEvents,
            InterServerEvents,
            SocketData
        >(fastify.server);
        this.#registerEventListeners();
    }

    /**
     * Creates a new MongoSocketIoServer.
     *
     * @param fastify
     * @param options
     * @return
     */
    static async create (
        fastify: FastifyInstance,
        options: DbOptions
    ): Promise<MongoSocketIoServer> {
        const mongoDb = await initializeMongoClient(options);
        return new MongoSocketIoServer(fastify, mongoDb);
    }

    /**
     * Registers event listeners on socket connection.
     */
    #registerEventListeners () {
        this.#io.on("connection", (socket) => {
            this.#fastify.log.info(`New socket connected with ID:${socket.id}`);
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
        this.#fastify.log.info(`Socket:${socket.id} disconnected`);
        const subscribedQueryIds = this.#subscribedQueryIdsMap.get(socket.id);

        if ("undefined" === typeof subscribedQueryIds) {
            return;
        }

        for (const queryId of subscribedQueryIds) {
            this.#unsubscribe(socket, queryId);
        }

        this.#subscribedQueryIdsMap.delete(socket.id);
        this.#fastify.log.debug(
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
    )
        : MongoWatcherCollection {
        let watcherCollection = this.#collections.get(collectionName);
        if ("undefined" === typeof watcherCollection) {
            watcherCollection = new MongoWatcherCollection(collectionName, this.#mongoDb);
            this.#fastify.log.debug(`Initialize Mongo watcher collection:${collectionName}.`);
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

        this.#fastify.log.debug(
            `Socket:${socket.id} requested query:${JSON.stringify(query)} ` +
            `with options:${JSON.stringify(options)} to collection:${collectionName}`
        );

        const hasCollection = await this.#hasCollection(collectionName);
        if (false === hasCollection) {
            this.#fastify.log.error(`Collection ${collectionName} does not exist in MongoDB`);
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
        this.#fastify.log.info(
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
            this.#fastify.log.error(`Query:${queryId} not found in query map`);

            return;
        }

        const queryParams: QueryParameters = getQuery(queryHash);

        const collection = this.#collections.get(queryParams.collectionName);
        if ("undefined" === typeof collection) {
            this.#fastify.log.error(`${queryParams.collectionName} is missing from server`);

            return;
        }

        const isLastSubscriber = collection.unsubscribe(queryId, socket.id);
        this.#fastify.log.info(`Socket:${socket.id} unsubscribed from query:${queryId}`);

        if (isLastSubscriber) {
            this.#fastify.log.debug(`Query:${queryId} deleted from query map.`);
            this.#queryIdToQueryHashMap.delete(queryId);
        }

        this.#fastify.log.debug(
            "Query ID to query hash map:" +
            ` ${JSON.stringify(Array.from(this.#queryIdToQueryHashMap.entries()))}`
        );

        if (false === collection.isReferenced()) {
            this.#fastify.log.debug(`Collection:${queryParams.collectionName}` +
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
        this.#fastify.log.debug(
            `Socket:${socket.id} requested unsubscription to query:${queryId}`
        );

        const subscribedQueryIds = this.#subscribedQueryIdsMap.get(socket.id);
        if ("undefined" === typeof subscribedQueryIds ||
            false === subscribedQueryIds.includes(queryId)
        ) {
            this.#fastify.log.error(`Socket ${socket.id} is not subscribed to ${queryId}`);

            return;
        }

        this.#unsubscribe(socket, queryId);
        await socket.leave(queryId.toString());

        removeItemFromArray(subscribedQueryIds, queryId);

        this.#fastify.log.debug(
            `Subscribed queryIDs map ${
                JSON.stringify(Array.from(this.#subscribedQueryIdsMap.entries()))}`
        );
    }
}

/**
 * A Fastify plugin callback for setting up the `MongoSocketIoServer`.
 *
 * @param app
 * @param options
 * @param options.database
 * @param options.host
 * @param options.port
 */
const MongoServerPlugin: FastifyPluginAsync<DbOptions> = async (
    app: FastifyInstance,
    options: DbOptions
) => {
    await MongoSocketIoServer.create(app, options);
};

export default fastifyPlugin(MongoServerPlugin);
