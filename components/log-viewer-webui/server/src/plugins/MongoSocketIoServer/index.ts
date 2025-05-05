/* eslint-disable max-lines */
/* eslint-disable no-warning-comments */
// TODO: Move listeners to a seperate file to reduce lines
// Reference: https://github.com/socketio/socket.io/blob/main/examples/basic-crud-application/server/lib/todo-management/todo.handlers.ts

import {
    FastifyInstance,
    FastifyPluginAsync,
} from "fastify";
import fastifyPlugin from "fastify-plugin";
import {
    Db,
} from "mongodb";
import {Server} from "socket.io";

import type {
    ClientToServerEvents,
    InterServerEvents,
    Response,
    ServerToClientEvents,
    SocketData,
} from "@common/index.js";
import MongoWatcherCollection from "./MongoWatcherCollection.js";
import {
    ConnectionId,
    DbOptions,
    MongoCustomSocket,
    QueryId,
    QueryParameters,
} from "./typings.js";
import {
    getQuery,
    getQueryHash,
    initializeMongoClient,
} from "./utils.js";


/**
 * Manages client interactions with MongoDB.
 *
 * TODO: In current implementation, all queries in a collection are sent using the same event. A
 * potential improvement would be to use different event names per query.
 */
class MongoSocketIoServer {
    #fastify: FastifyInstance;

    #io: Server<ClientToServerEvents, ServerToClientEvents, InterServerEvents, SocketData>;

    // Collections with active queries
    #collections: Map<string, MongoWatcherCollection> = new Map();

    // Mapping of active queries to their hash
    #queryIdtoQueryHashMap: Map<QueryId, string> = new Map();

    // List of subscribed query IDs for each connection. Duplicate query IDs for the same connection ID
    // are allowed since the connection can subscribe to the same query multiple times.
    #subscribedQueryIdsMap: Map<ConnectionId, QueryId[]> = new Map();

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
            socket.on("collection::init", this.#collectionInitListener.bind(this, socket));
            socket.on(
                "collection::find::toReactiveArray",
                this.#collectionFindToReactiveArrayListener.bind(this, socket)
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
        this.#fastify.log.info(`Subscriber map: ${JSON.stringify(Array.from(this.#subscribedQueryIdsMap.entries()))}`);
    }

    /**
     * Checks if a collection exists in the MongoDB database.
     *
     * @param collectionName
     * @return Whether the collection exists.
     */
    async #collectionExists (collectionName: string): Promise<boolean> {
        const collections = await this.#mongoDb.listCollections().toArray();
        return collections.some((collection) => collection.name === collectionName);
    }

    /**
     * Listener for initializing a connection to a collection.
     *
     * @param socket
     * @param requestArgs
     * @param requestArgs.collectionName
     * @param callback
     */
    async #collectionInitListener (
        socket: MongoCustomSocket,
        requestArgs: {collectionName: string},
        callback:(res: Response<void>) => void
    ): Promise<void> {
        const {collectionName} = requestArgs;
        this.#fastify.log.info(
            `Socket:${socket.id} requested init of collection:${collectionName}`
        );

        const exists = await this.#collectionExists(collectionName);
        if (false === exists) {
            this.#fastify.log.error(`Collection ${collectionName} does not exist in MongoDB`);
            callback({
                error: `Collection ${collectionName} does not exist in MongoDB`,
            });

            return;
        }

        socket.data.collectionName = collectionName;
    }


    /**
     * Adds the query ID to the connection's subscribed query IDs.
     *
     * @param queryId
     * @param socketId
     */
    #AddQueryIdToSubscribedList (queryId: QueryId, socketId: ConnectionId) {
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
        for (const [queryId, hash] of this.#queryIdtoQueryHashMap.entries()) {
            if (hash === queryHash) {
                return queryId;
            }
        }

        let queryId = 0;
        if (0 === this.#queryIdtoQueryHashMap.size) {
            this.#queryIdtoQueryHashMap.set(queryId, queryHash);
        } else {
            const maxKey = Math.max(...Array.from(this.#queryIdtoQueryHashMap.keys()));
            queryId = maxKey + 1;
            this.#queryIdtoQueryHashMap.set(queryId, queryHash);
        }

        return queryId;
    }

    /**
     * Listener for subscribing to a reactive find query. The client will receive updates whenever
     * the query results change.
     *
     * @param socket
     * @param requestArgs
     * @param requestArgs.query
     * @param requestArgs.options
     * @param callback
     */
    async #collectionFindToReactiveArrayListener (
        socket: MongoCustomSocket,
        requestArgs: {query: object; options: object},
        callback: (res: Response<{queryId: number; initialDocuments: object[]}>) => void
    ): Promise<void> {
        const {query, options} = requestArgs;
        const {collectionName} = socket.data;
        if (!collectionName) {
            this.#fastify.log.error("Collection name is undefined");

            return;
        }

        this.#fastify.log.info(
            `Socket:${socket.id} requested query:${JSON.stringify(query)} with options:${JSON.stringify(options)} to collection:${collectionName}`
        );


        let watcherCollection = this.#collections.get(collectionName);
        if ("undefined" === typeof watcherCollection) {
            watcherCollection = new MongoWatcherCollection(collectionName, this.#mongoDb);
            this.#collections.set(collectionName, watcherCollection);
        }

        const queryParameters: QueryParameters = {
            collectionName,
            query,
            options,
        };

        const queryId = this.#getQueryId(queryParameters);


        const hasWatcher = watcherCollection.hasWatcher(queryId);

        if (false === hasWatcher) {
            const emitUpdate = (queryId: QueryId, data: object[]) => {
                this.#io.to(`${queryId}`).emit("collection::find::update", {queryId, data});
            };

            watcherCollection.createWatcher(queryParameters, queryId, emitUpdate);
        }

        await watcherCollection.subscribe(queryId, socket);

        const initialDocuments = await watcherCollection.find(queryParameters);

        callback({data: {queryId, initialDocuments}});
        this.#AddQueryIdToSubscribedList(queryId, socket.id);
        this.#fastify.log.info(
            `Socket:${socket.id} subscribed to queryID:${queryId}.`
        );

    }

    /**
     * Unsubscribes from a query.
     *
     * @param socket
     * @param queryId
     */
    #unsubscribe (socket: MongoCustomSocket, queryId: number) {
        const queryHash: string | undefined = this.#queryIdtoQueryHashMap.get(queryId);
        if ("undefined" === typeof queryHash) {
            this.#fastify.log.error(`QueryId ${queryId} not found in query map`);

            return;
        }

        const queryParams: QueryParameters = getQuery(queryHash);

        const collection = this.#collections.get(queryParams.collectionName);
        if ("undefined" === typeof collection) {
            this.#fastify.log.error(`${queryParams.collectionName} is missing from server`);

            return;
        }

        const lastSubscriber = collection.unsubcribe(queryId, socket.id);
        this.#fastify.log.info(`Socket ${socket.id} unsubscribed from query ${queryId}`);
        this.#fastify.log.info(`last subscriber? ${lastSubscriber}`);

        if (lastSubscriber) {
            this.#fastify.log.info(`QueryID:${queryId} deleted from query map.`);
            this.#queryIdtoQueryHashMap.delete(queryId);
        }

        if (false === collection.isReferenced()) {
            this.#fastify.log.info(`Collection:${queryParams.collectionName} deallocated from server.`);
            this.#collections.delete(queryParams.collectionName);
        }
        this.#fastify.log.info(`queryID map: ${JSON.stringify(Array.from(this.#queryIdtoQueryHashMap.entries()))}`);


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
        this.#fastify.log.info(
            `Socket ${socket.id} requested unsubscription to QueryId ${queryId}`
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

        // Remove one queryID.
        const index = subscribedQueryIds.indexOf(queryId);
        if (-1 !== index) {
            subscribedQueryIds.splice(index, 1);
        }

        this.#fastify.log.info(`Subscriber map: ${JSON.stringify(Array.from(this.#subscribedQueryIdsMap.entries()))}`);
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
