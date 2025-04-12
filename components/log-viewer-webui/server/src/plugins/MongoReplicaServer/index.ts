import {FastifyInstance} from "fastify";
import fastifyPlugin from "fastify-plugin";
import {Server as HttpServer} from "http";
import {
    Db,
    Document,
    MongoClient,
} from "mongodb";
import {
    Server,
    Socket,
} from "socket.io";

import MongoReplicaServerCollection from "./MongoReplicaServerCollection.js";


// Define the events that the client can emit to the server
type ClientToServerEvents = {
    "disconnect": (reqArgs: never) => void;
    "collection::init": (reqArgs: {
        collectionName: string;
    }) => void;
    "collection::find::toArray": (
        reqArgs: {
            query: object;
            options: object;
        },
        callback: (respArgs: {
            data: Document[];
        } | {
            error: string;
        }) => void
    ) => Promise<void>;
    "collection::find::toReactiveArray": (
        reqArgs: {
            query: object;
            options: object;
        },
        callback: (respArgs: {
            queryHash: string;
        } | {
            error: string;
        }) => void
    ) => Promise<void>;
    "collection::find::unsubscribe": (
        reqArgs: {
            queryHash: string;
        }
    ) => void;
};

// Define the events that the server can emit to the client
interface ServerToClientEvents {
    "collection::find::update": (respArgs: {
        data: Document[];
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
class MongoReplicaServer {
    #fastify: FastifyInstance; // Fastify instance for logging and handling requests

    #collections: Map<string, MongoReplicaServerCollection>; // Map of collections being managed

    readonly #mongoDb: Db; // MongoDB database instance

    /**
     * Creates an instance of MongoReplicaServer.
     *
     * @param fastify.fastify
     * @param fastify The Fastify instance.
     * @param mongoDb The MongoDB database instance.
     * @param fastify.mongoDb
     */
    constructor ({fastify, mongoDb}: {fastify: FastifyInstance; mongoDb: Db}) {
        this.#fastify = fastify;
        this.#collections = new Map();
        this.#mongoDb = mongoDb;
        this.#initializeSocketServer(fastify.server);
    }

    /**
     * Creates a new instance of MongoReplicaServer by initializing the MongoDB client.
     *
     * @param fastify.fastify
     * @param fastify The Fastify instance.
     * @param database The name of the database to connect to.
     * @param host The host of the MongoDB server.
     * @param port The port of the MongoDB server.
     * @param fastify.database
     * @param fastify.host
     * @param fastify.port
     * @return A promise that resolves to a MongoReplicaServer instance.
     */
    static async create ({
        fastify,
        database,
        host,
        port,
    }: {
        fastify: FastifyInstance;
        database: string;
        host: string;
        port: string;
    }): Promise<MongoReplicaServer> {
        const mongoDb = await MongoReplicaServer.initializeMongoClient({database, host, port});

        return new MongoReplicaServer({fastify, mongoDb});
    }

    /**
     * Initializes the MongoDB client and connects to the specified database.
     *
     * @param database.database
     * @param database The name of the database to connect to.
     * @param host The host of the MongoDB server.
     * @param port The port of the MongoDB server.
     * @param database.host
     * @param database.port
     * @return A promise that resolves to the MongoDB database instance.
     */
    static async initializeMongoClient (
        {database, host, port}: {database: string; host: string; port: string}
    ): Promise<Db> {
        const mongoUri = `mongodb://${host}:${port}`;
        const mongoClient = new MongoClient(mongoUri);
        try {
            await mongoClient.connect();

            return mongoClient.db(database);
        } catch (e) {
            throw new Error("MongoDB connection error", {cause: e});
        }
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
            socket.on("disconnect", this.#getCollectionDisconnectListener(socket));
            socket.on("collection::init", this.#getCollectionInitListener(socket));
            socket.on(
                "collection::find::toReactiveArray",
                this.#getCollectionFindToReactiveArrayListener(socket)
            );
            socket.on(
                "collection::find::unsubscribe",
                this.#getCollectionFindUnsubscribeListener(socket)
            );
        });
    }

    /**
     * Creates a listener for socket disconnection events.
     * Cleans up references to collections when a socket disconnects.
     *
     * @param socket The socket instance that is disconnecting.
     * @return A function that handles the disconnection event.
     */
    #getCollectionDisconnectListener (socket: CustomSocket) {
        return () => {
            this.#fastify.log.info(`Socket disconnected: ${socket.id}`);
            const {collectionName} = socket.data as {collectionName: string};
            const collection = this.#collections.get(collectionName);
            if ("undefined" !== typeof collection) {
                collection.refRemove();
                if (!collection.isReferenced()) {
                    this.#fastify.log.info(`Collection ${collectionName} removed`);
                    this.#collections.delete(collectionName);
                }
            }
        };
    }

    /**
     * Creates a listener for initializing a collection.
     * Adds a reference to the collection when requested by the client.
     *
     * @param socket The socket instance that is requesting the collection.
     * @return A function that handles the collection initialization event.
     */
    #getCollectionInitListener (socket: CustomSocket): ClientToServerEvents["collection::init"] {
        return ({collectionName}) => {
            this.#fastify.log.info(`Collection name ${collectionName} requested`);

            let collection = this.#collections.get(collectionName);
            if ("undefined" === typeof collection) {
                collection = new MongoReplicaServerCollection(
                    this.#mongoDb,
                    collectionName
                );
                this.#collections.set(collectionName, collection);
            }
            collection.refAdd();

            socket.data.collectionName = collectionName;
        };
    }

    /**
     * Creates a listener for subscribing to a reactive array of documents.
     * Emits updates to the client when changes occur in the collection.
     *
     * @param socket The socket instance that is requesting the subscription.
     * @return A function that handles the subscription event.
     */
    #getCollectionFindToReactiveArrayListener (socket: CustomSocket)
        : ClientToServerEvents["collection::find::toReactiveArray"] {
        return async ({query, options}, callback) => {
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
            callback({queryHash});
            // eslint-disable-next-line @typescript-eslint/no-misused-promises
            watcher.changeStream.on("change", async () => {
                // eslint-disable-next-line no-warning-comments
                // FIXME: this should be debounced
                socket.emit("collection::find::update", {
                    data: await collection.find(query, options).toArray(),
                });
            });

            socket.emit("collection::find::update", {
                data: await collection.find(query, options).toArray(),
            });
        };
    }

    /**
     * Creates a listener for unsubscribing from a reactive array of documents.
     * Removes the watcher for the specified query hash.
     *
     * @param socket The socket instance that is requesting the unsubscription.
     * @return A function that handles the unsubscription event.
     */
    #getCollectionFindUnsubscribeListener (socket: CustomSocket)
        : ClientToServerEvents["collection::find::unsubscribe"] {
        return ({queryHash}) => {
            const {collectionName} = socket.data as {collectionName: string};
            this.#fastify.log.info(`Collection name ${collectionName} requested unsubscription`);
            const collection = this.#collections.get(collectionName);

            if ("undefined" === typeof collection) {
                return;
            }

            collection.removeWatcher(queryHash, socket.id);
        };
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
const MongoReplicaServerPlugin = async (
    app: FastifyInstance,
    options: {host: string; port: number; database: string}
) => {
    await MongoReplicaServer.create({
        fastify: app,
        host: options.host,
        port: options.port.toString(),
        database: options.database,
    });
};

// Export the plugin wrapped in fastify-plugin for use in Fastify applications
export default fastifyPlugin(MongoReplicaServerPlugin);
