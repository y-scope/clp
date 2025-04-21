/* eslint-disable max-lines */
import {FastifyInstance} from "fastify";
import fastifyPlugin from "fastify-plugin";
import {Server as HttpServer} from "http";
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
import {initializeMongoClient} from "./utils.js";


const updateTimeout = 500;

// Define the events that the client can emit to the server
type ClientToServerEvents = {
    "disconnect": () => void;
    "collection::init": (reqArgs: {
        collectionName: string;
    }) => void;
    "collection::find::toArray": (
        reqArgs: {
            query: Filter<Document>;
            options: FindOptions;
        },
        callback: (respArgs: {
            data: Document[];
        } | {
            error: string;
        }) => void
    ) => Promise<void>;
    "collection::find::toReactiveArray": (
        reqArgs: {
            query: Filter<Document>;
            options: FindOptions;
        },
        callback: (respArgs: {
            queryId: number;
        } | {
            error: string;
        }) => void
    ) => Promise<void>;
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
    "error::query": (respArgs: {
        queryId: number;
        message: string;
    }) => void;
    "error::collection": (respArgs: {
        collectionName: string;
        message: string;
    }) => void;
}

type InterServerEvents = object;

// Define the data structure associated with each socket connection
interface SocketData {
    collectionName: string;
}

// Define a custom socket type with specific events and data
type CustomSocket = Socket<
    ClientToServerEvents,
    ServerToClientEvents,
    InterServerEvents,
    SocketData
>;

/**
 * Class representing a MongoDB replica server with real-time capabilities.
 * This class manages socket connections and interactions with MongoDB collections.
 */
class FastifyMongoServer {
    // Fastify instance for logging and handling requests
    #fastify: FastifyInstance;

    // Map of collections being managed
    #collections: Map<string, MongoServerCollection> = new Map();

    // Map of unique queries being managed
    #queryMap: Map<number, {collectionName: string; queryHash: string}> = new Map();

    // Map of active socket connections and the collections they are subscribed to
    #connections: Map<string, string[]> = new Map();

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
        this.#initializeSocketServer(fastify.server);
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
    ): Promise<FastifyMongoServer> {
        const mongoDb = await initializeMongoClient(
            database,
            host,
            port.toString()
        );

        return new FastifyMongoServer(fastify, mongoDb);
    }

    /**
     * Initializes the Socket.IO server and sets up event listeners for socket connections.
     *
     * @param httpServer The HTTP server instance to attach the Socket.IO server to.
     */
    #initializeSocketServer (httpServer: HttpServer) {
        const io = new Server<
            ClientToServerEvents,
            ServerToClientEvents,
            InterServerEvents,
            SocketData
        >(httpServer);

        io.on("connection", (socket) => {
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
        const connections = this.#connections.get(socket.id);
        if ("undefined" !== typeof connections) {
            for (const collectionName of connections) {
                for (const [key, value] of this.#queryMap.entries()) {
                    console.log(`key: ${key} value: ${JSON.stringify(value)}`);
                    if (collectionName === value.collectionName) {
                        await this.#unsubscribe(collectionName, key, socket);
                    }
                }
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
    #addConnection (socketId: string, collectionName: string) {
        if (this.#connections.has(socketId)) {
            const collections = this.#connections.get(socketId);
            if (false === collections?.includes(collectionName)) {
                collections.push(collectionName);
            }
        } else {
            this.#connections.set(socketId, [collectionName]);
        }
    }

    /**
     * Listener for initializing a collection.
     * Adds a reference to the collection when requested by the client.
     *
     * @param socket The socket instance that is requesting the collection.
     * @param reqArgs
     * @param reqArgs.collectionName
     */
    #collectionInitListener (socket: CustomSocket, reqArgs: {collectionName: string}): void {
        const {collectionName} = reqArgs;
        this.#fastify.log.info(`Collection name ${collectionName} requested`);

        // Check if the collection exists
        this.#collectionExists(collectionName).then((exists) => {
            if (!exists) {
                this.#fastify.log.error(`Collection ${collectionName} does not exist`);
                socket.emit(
                    "error::collection",
                    {
                        collectionName: collectionName,
                        message: `Collection ${collectionName} does not exist`,
                    }
                );

                return;
            }

            let collection = this.#collections.get(collectionName);
            if ("undefined" === typeof collection) {
                collection = new MongoServerCollection(this.#mongoDb, collectionName);
                this.#collections.set(collectionName, collection);
            }

            this.#addConnection(socket.id, collectionName);
            collection.refAdd();
            socket.data.collectionName = collectionName;
        })
            .catch((error: unknown) => {
                this.#fastify.log
                    .error(`Error checking collection existence: 
                        ${collectionName} - ${(error as Error).message}`);
                socket.emit(
                    "error::collection",
                    {
                        collectionName: collectionName,
                        message: "An error occurred while checking the collection.",
                    }
                );
            });
    }

    /**
     * Function to return queryId for a specific collectionName and queryHash
     * from the queryMap. If not found, it creates a new entry in the map.
     *
     * @param collectionName
     * @param queryHash
     * @return The key associated with the collectionName and queryHash.
     */
    #getQueryId (collectionName: string, queryHash: string): number {
        for (const [key, value] of this.#queryMap.entries()) {
            if (value.collectionName === collectionName && value.queryHash === queryHash) {
                return key;
            }
        }

        let queryId = 0;
        if (0 === this.#queryMap.size) {
            this.#queryMap.set(0, {collectionName, queryHash});
        } else {
            const maxKey = Math.max(...Array.from(this.#queryMap.keys()));
            queryId = maxKey + 1;
            this.#queryMap.set(queryId, {collectionName, queryHash});
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
    // eslint-disable-next-line max-statements
    async #collectionFindToReactiveArrayListener (
        socket: CustomSocket,
        reqArgs: {query: Filter<Document>; options: FindOptions},
        callback: (respArgs: {queryId: number} | {error: string}) => void
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

        const {queryHash, watcher} = collection.getWatcher(query, options, socket.id);
        const queryId = this.#getQueryId(collectionName, queryHash);
        console.log(`queryID: ${queryId} queryHash: ${queryHash}`);
        callback({queryId});

        // Join the room corresponding to the query hash
        try {
            console.log(`Socket ${socket.id} joining room ${queryId}`);
            await socket.join(`${queryId}`);
        } catch (error) {
            this.#fastify.log
                .error(`Error joining room ${queryId}: ${(error as Error).message}`);
            socket.emit(
                "error::query",
                {
                    queryId: queryId,
                    message: "An error occurred while joining the socket.io room.",
                }
            );

            return;
        }

        let lastEmitTime = 0;

        const emitUpdate = async () => {
            const currentTime = Date.now();
            if (updateTimeout <= currentTime - lastEmitTime) {
                lastEmitTime = currentTime;
                socket.to(`${queryId}`).emit("collection::find::update", {
                    queryId: queryId,
                    data: await collection.find(query, options).toArray(),
                });
            }
        };

        // eslint-disable-next-line @typescript-eslint/no-misused-promises
        watcher.changeStream.on("change", emitUpdate);

        socket.emit("collection::find::update", {
            queryId: queryId,
            data: await collection.find(query, options).toArray(),
        });
    }

    async #unsubscribe (collectionName: string, queryId: number, socket: CustomSocket) {
        this.#fastify.log.info(`Collection name ${collectionName} requested unsubscription`);
        const collection = this.#collections.get(collectionName);

        if ("undefined" === typeof collection) {
            return;
        }

        await socket.leave(`${queryId}`);

        const queryHash = this.#queryMap.get(queryId)?.queryHash;
        console.log(this.#queryMap);
        if ("undefined" === typeof queryHash) {
            this.#fastify.log.error(`QueryId ${queryId} not found in queryMap`);
            socket.emit(
                "error::query",
                {
                    queryId: queryId,
                    message: `QueryId ${queryId} not found in queryMap`,
                }
            );

            return;
        }
        const watcherRemoved = collection.removeWatcher(queryHash, socket.id);

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
        console.log(`collectionName: ${collectionName} queryId: ${queryId}`);
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
    await FastifyMongoServer.create(app, options.database, options.host, options.port);
};

// Export the plugin wrapped in fastify-plugin for use in Fastify applications
export default fastifyPlugin(MongoServerPlugin);
