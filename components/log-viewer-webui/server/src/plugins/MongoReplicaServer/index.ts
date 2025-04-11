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

interface ServerToClientEvents {
    "collection::find::update": (respArgs: {
        data: Document[];
    }) => void;
}

type InterServerEvents = object;

interface SocketData {
    collectionName: string;
}

type CustomSocket = Socket<
    ClientToServerEvents,
    ServerToClientEvents,
    InterServerEvents,
    SocketData
>;

class MongoReplicaServer {
    #fastify: FastifyInstance;

    #collections: Map<string, MongoReplicaServerCollection>;

    #mongoDb: Db;

    constructor ({fastify, mongoDb}: {fastify: FastifyInstance; mongoDb: Db}) {
        this.#fastify = fastify;
        this.#collections = new Map();
        this.#mongoDb = mongoDb;
        this.#initializeSocketServer(fastify.server);
    }

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

            const {queryHash, watcher} = collection.getWatcher(query, options);
            callback({queryHash});
            // eslint-disable-next-line @typescript-eslint/no-misused-promises
            watcher.on("change", async () => {
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

    #getCollectionFindUnsubscribeListener (socket: CustomSocket)
        : ClientToServerEvents["collection::find::unsubscribe"] {
        return ({queryHash}) => {
            const {collectionName} = socket.data as {collectionName: string};
            this.#fastify.log.info(`Collection name ${collectionName} requested unsubscription`);
            const collection = this.#collections.get(collectionName);

            if ("undefined" === typeof collection) {
                return;
            }

            collection.removeWatcher(queryHash);
        };
    }
}

/**
 * MongoDB replica set plugin for Fastify.
 *
 * @param app
 * @param options
 * @param options.database
 * @param options.host
 * @param options.port
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


export default fastifyPlugin(MongoReplicaServerPlugin);
