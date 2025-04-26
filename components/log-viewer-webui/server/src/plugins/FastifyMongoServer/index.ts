/* eslint-disable max-lines */
import {FastifyInstance} from "fastify";
import fastifyPlugin from "fastify-plugin";
import {
    Db,
    Document,
    type Filter,
    type FindOptions,
} from "mongodb";
import {
    Server,
    Socket,
} from "socket.io";

import MongoServerCollection from "./MongoServerCollection.js";
import {
    getQueryHash,
    initializeMongoClient,
} from "./utils.js";


interface Error {
    collectionName?: string;
    error: string;
    queryId?: number;
}

interface Success<T> {
    data: T;
}

type Response<T> = Error | Success<T>;

// Define the events that the client can emit to the server
type ClientToServerEvents = {
    "disconnect": () => void;
    "collection::init": (
        reqArgs: {
            collectionName: string;
        },
        callback: (res: Error) => void
    ) => void;
    "collection::find::toArray": (
        reqArgs: {
            query: Filter<Document>;
            options: FindOptions;
        },
        callback: (res: Response<{data: Document[]}>) => void) => Promise<void>;
    "collection::find::toReactiveArray": (
        reqArgs: {
            query: Filter<Document>;
            options: FindOptions;
        },
        callback: (res: Response<{queryId: number}>) => void) => Promise<void>;
    "collection::find::unsubscribe": (
        reqArgs: {
            queryId: number;
        }
    ) => Promise<void>;
};

// Define the events that the server can emit to the client
interface ServerToClientEvents {
    "collection::find::update": (respArgs: {
        queryId: number;
        data: Document[];
    }) => void;
}

// Define the data structure associated with each socket connection
interface SocketData {
    collectionName: string;
}

// Define a custom socket type with specific events and data
type CustomSocket = Socket<
    ClientToServerEvents,
    ServerToClientEvents,
    SocketData
>;

/**
 * Class representing a MongoDB replica server with real-time capabilities.
 * This class manages socket connections and interactions with MongoDB collections.
 */
class MongoSocketIoServer {
    // Fastify instance for logging and handling requests
    #fastify: FastifyInstance;

    #io: Server<ClientToServerEvents, ServerToClientEvents, SocketData>;

    // Map of collections being managed
    #collections: Map<string, MongoServerCollection> = new Map();

    // Map of unique queries being managed (ie. queryId to queryHash)
    #queryMap: Map<number, string> = new Map();

    // Map of active socket connections and the collections they are subscribed to
    #connectionsToCollectionMap: Map<string, string[]> = new Map();

    // Map of active socket connections and the queryIds they are subscribed to
    #connectionsToQueryIdMap: Map<string, number[]> = new Map();

    // Map of queryIds and collection they are too
    #queryIdToCollectionNameMap: Map<number, string> = new Map();

    // MongoDB database instance
    readonly #mongoDb: Db;

    /**
     * Creates an instance of MongoReplicaServer.
     *
     * @param fastify The Fastify instance.
     * @param mongoDb The MongoDB database instance.
     */
    constructor (fastify: FastifyInstance, mongoDb: Db) {
        this.#fastify = fastify;
        this.#mongoDb = mongoDb;
        this.#io = new Server<
            ClientToServerEvents,
            ServerToClientEvents,
            SocketData
        >(fastify.server);

        this.#startSocketListeners();
    }

    /**
     * Creates a new instance of MongoReplicaServer by initializing the MongoDB client.
     *
     * @param fastify The Fastify instance.
     * @param database The name of the database to connect to.
     * @param host The host of the MongoDB server.
     * @param port The port of the MongoDB server.
     * @return A promise that resolves to a MongoReplicaServer instance.
     */
    static async create (
        fastify: FastifyInstance,
        database: string,
        host: string,
        port: number
    ): Promise<MongoSocketIoServer> {
        const mongoDb = await initializeMongoClient(
            database,
            host,
            port.toString()
        );

        return new MongoSocketIoServer(fastify, mongoDb);
    }

    /**
     * Sets up event listeners for socket connections.
     */
    #startSocketListeners () {
        this.#io.on("connection", (socket) => {
            this.#fastify.log.info(`Socket connected: ${socket.id}`);
            socket.on("disconnect", this.#collectionDisconnectListener.bind(this, socket));
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
     * Cleans up references to collections when a socket disconnects.
     *
     * @param socket The socket instance that is disconnecting.
     */
    async #collectionDisconnectListener (socket: CustomSocket) {
        this.#fastify.log.info(`Socket disconnected: ${socket.id}`);
        const queryConnections = this.#connectionsToQueryIdMap.get(socket.id);
        const collectionConnections = this.#connectionsToCollectionMap.get(socket.id);
        if ("undefined" !== typeof queryConnections) {
            for (const queryId of queryConnections) {
                const collectionName = this.#queryIdToCollectionNameMap.get(queryId);
                if ("undefined" === typeof collectionName) {
                    this.#fastify.log.error(`QueryId ${queryId} not found`);
                    continue;
                }
                this.#fastify.log.info(`QueryId ${queryId} requested unsubscription`);
                await this.#unsubscribe(collectionName, queryId, socket);
            }
        }
        if ("undefined" !== typeof collectionConnections) {
            for (const collectionName of collectionConnections) {
                const collection = this.#collections.get(collectionName);
                if ("undefined" !== typeof collection) {
                    collection.refRemove();
                    if (!collection.isReferenced()) {
                        this.#fastify.log.info(`Collection ${collectionName} removed`);
                        this.#collections.delete(collectionName);
                    }
                }
            }
        }
    }

    // Method to check if a collection exists
    async #collectionExists (collectionName: string): Promise<boolean> {
        const collections = await this.#mongoDb.listCollections().toArray();
        return collections.some((collection) => collection.name === collectionName);
    }

    /**
     * Add a connection for a specific socket ID and collection name.
     *
     * @param socketId The ID of the socket connection.
     * @param collectionName The name of the collection being subscribed to.
     */
    #addCollectionConnection (socketId: string, collectionName: string) {
        if (this.#connectionsToCollectionMap.has(socketId)) {
            const collections = this.#connectionsToCollectionMap.get(socketId);
            if (false === collections?.includes(collectionName)) {
                collections.push(collectionName);
            }
        } else {
            this.#connectionsToCollectionMap.set(socketId, [collectionName]);
        }
    }

    /**
     * Listener for initializing a collection.
     * Adds a reference to the collection when requested by the client.
     *
     * @param socket The socket instance that is requesting the collection.
     * @param reqArgs
     * @param reqArgs.collectionName
     * @param callback
     */
    #collectionInitListener (
        socket: CustomSocket,
        reqArgs: {collectionName: string},
        callback:(res: Error) => void
    ): void {
        const {collectionName} = reqArgs;
        this.#fastify.log.info(`Collection name ${collectionName} requested`);

        // Check if the collection exists
        this.#collectionExists(collectionName).then((exists) => {
            if (!exists) {
                this.#fastify.log.error(`Collection ${collectionName} does not exist`);
                callback({
                    collectionName: collectionName,
                    error: `Collection ${collectionName} does not exist`,
                });

                return;
            }

            let collection = this.#collections.get(collectionName);
            if ("undefined" === typeof collection) {
                collection = new MongoServerCollection(collectionName, this.#io, this.#mongoDb);
                this.#collections.set(collectionName, collection);
            }

            this.#addCollectionConnection(socket.id, collectionName);
            collection.refAdd();

            // eslint-disable-next-line @typescript-eslint/no-unsafe-member-access
            socket.data.collectionName = collectionName;
        })
            .catch((error: unknown) => {
                this.#fastify.log
                    .error(`Error checking collection existence: 
                        ${collectionName} - ${(error as Error).error}`);
                callback({
                    collectionName: collectionName,
                    error: "An error occurred while checking the collection.",
                });
            });
    }


    /**
     * Add a connection for a specific socket ID and queryId.
     *
     * @param collectionName The name of the collection being subscribed to.
     * @param queryId The queryID of the query being subscribed to.
     * @param socketId The ID of the socket connection.
     */
    #addQueryConnection (collectionName: string, queryId: number, socketId: string) {
        if (this.#connectionsToQueryIdMap.has(socketId)) {
            const queries = this.#connectionsToQueryIdMap.get(socketId);
            if (false === queries?.includes(queryId)) {
                queries.push(queryId);
            }
        } else {
            this.#connectionsToQueryIdMap.set(socketId, [queryId]);
        }
        if (false === this.#queryIdToCollectionNameMap.has(queryId)) {
            this.#queryIdToCollectionNameMap.set(queryId, collectionName);
        }
    }

    /**
     * Function to return queryId for a specific collectionName and queryHash
     * from the queryMap. If not found, it creates a new entry in the map.
     *
     * @param collectionName
     * @param query
     * @param options
     * @return The key associated with the collectionName, query, and options.
     */
    #getQueryId (collectionName: string, query: Filter<Document>, options: FindOptions): number {
        const queryHash = getQueryHash(collectionName, query, options);
        for (const [id, hash] of this.#queryMap.entries()) {
            if (hash === queryHash) {
                return id;
            }
        }

        let queryId = 0;
        if (0 === this.#queryMap.size) {
            this.#queryMap.set(0, queryHash);
        } else {
            const maxKey = Math.max(...Array.from(this.#queryMap.keys()));
            queryId = maxKey + 1;
            this.#queryMap.set(queryId, queryHash);
        }

        return queryId;
    }

    /**
     * Listener for subscribing to a reactive array of documents.
     * Emits updates to the client every 500ms when changes occur in the collection.
     *
     * @param socket The socket instance that is requesting the subscription.
     * @param reqArgs
     * @param reqArgs.query
     * @param reqArgs.options
     * @param callback
     */
    async #collectionFindToReactiveArrayListener (
        socket: CustomSocket,
        reqArgs: {query: Filter<Document>; options: FindOptions},
        callback: (res: Response<{queryId: number}>) => void
    ): Promise<void> {
        const {query, options} = reqArgs;
        const {collectionName} = socket.data as {collectionName: string};
        this.#fastify.log.info(
            `Collection name ${collectionName} requested subscription`
        );
        const collection = this.#collections.get(collectionName);

        if ("undefined" === typeof collection) {
            callback({error: "Collection not initialized"});

            return;
        }

        const queryId = this.#getQueryId(collectionName, query, options);

        collection.getWatcher(query, options, queryId, socket.id);

        callback({data: {queryId}});

        // Join the room corresponding to the query hash
        try {
            await socket.join(`${queryId}`);
        } catch (error) {
            this.#fastify.log
                .error(`Error joining room ${queryId}: ${(error as Error).error}`);
            callback({
                error: "An error occurred while joining the socket.io room.",
                queryId: queryId,
            });

            return;
        }

        this.#addQueryConnection(collectionName, queryId, socket.id);

        socket.emit("collection::find::update", {
            queryId: queryId,
            data: await collection.find(query, options).toArray(),
        });
    }

    /**
     * Unsubscribes from a reactive array of documents.
     * Removes the watcher for the specified query hash.
     *
     * @param collectionName The name of the collection being unsubscribed from.
     * @param queryId The queryID of the query being unsubscribed from.
     * @param socket The socket instance that is requesting the unsubscription.
     */
    async #unsubscribe (collectionName: string, queryId: number, socket: CustomSocket) {
        this.#fastify.log.info(`QueryId ${queryId} requested unsubscription`);
        const queryIds = this.#connectionsToQueryIdMap.get(socket.id);
        if ("undefined" === typeof queryIds) {
            return;
        }

        if (false === queryIds.includes(queryId)) {
            return;
        }

        const collection = this.#collections.get(collectionName);
        if ("undefined" === typeof collection) {
            return;
        }

        await socket.leave(`${queryId}`);

        const queryHash = this.#queryMap.get(queryId);
        if ("undefined" === typeof queryHash) {
            this.#fastify.log.error(`QueryId ${queryId} not found in queryMap`);

            return;
        }
        const watcherRemoved = collection.removeWatcher(queryId, socket.id);

        if (watcherRemoved) {
            this.#queryMap.delete(queryId);
        }
    }

    /**
     * Listener for unsubscribing from a reactive array of documents.
     * Removes the watcher for the specified query hash.
     *
     * @param socket The socket instance that is requesting the unsubscription.
     * @param reqArgs
     * @param reqArgs.queryId
     */
    async #collectionFindUnsubscribeListener (
        socket: CustomSocket,
        reqArgs: {queryId: number}
    ): Promise<void> {
        const {queryId} = reqArgs;
        const {collectionName} = socket.data as {collectionName: string};
        await this.#unsubscribe(collectionName, queryId, socket);
    }
}

/**
 * MongoDB replica set plugin for Fastify.
 * This plugin integrates the MongoReplicaServer into a Fastify application.
 *
 * @param app The Fastify instance.
 * @param options Configuration options for the plugin.
 * @param options.database The name of the database to connect to.
 * @param options.host The host of the MongoDB server.
 * @param options.port The port of the MongoDB server.
 */
const MongoServerPlugin = async (
    app: FastifyInstance,
    options: {host: string; port: number; database: string}
) => {
    await MongoSocketIoServer.create(app, options.database, options.host, options.port);
};

// Export the plugin wrapped in fastify-plugin for use in Fastify applications
export default fastifyPlugin(MongoServerPlugin);
