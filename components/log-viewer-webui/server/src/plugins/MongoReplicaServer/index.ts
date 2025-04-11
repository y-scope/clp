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


interface EventHandlerTypes {
    "collection::init": {
        reqArgs: {collectionName: string};
        respArgs: never;
    };
    "disconnect": {
        reqArgs: never;
        respArgs: never;
    };
    "collection::find::toArray": {
        reqArgs: {query: object; options: object};
        respArgs: {data: Document[]} | {error: string};
    };
    "collection::find::toReactiveArray": {
        reqArgs: {query: object; options: object};
        respArgs: {queryHash: string} | {error: string};
    };
    "collection::find::unsubscribe": {
        reqArgs: {queryHash: string};
        respArgs: never;
    };
}

type EventListener<E extends keyof EventHandlerTypes> =
  EventHandlerTypes[E]["respArgs"] extends never
      ? (reqArgs: EventHandlerTypes[E]["reqArgs"]) => void | Promise<void>
      : (
          reqArgs: EventHandlerTypes[E]["reqArgs"],
          callback: (respArgs: EventHandlerTypes[E]["respArgs"]) => void
      ) => void | Promise<void>;


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
        const io = new Server(httpServer);

        io.on("connection", (socket) => {
            this.#fastify.log.info(`Socket connected: ${socket.id}`);
            ([
                {
                    event: "disconnect",
                    listener: this.#getCollectionDisconnectListener(socket),
                },
                {
                    event: "collection::init",
                    listener: this.#getCollectionInitListener(socket),
                },
                {
                    event: "collection::find::toArray",
                    listener: this.#getCollectionFindToArrayListener(socket),
                },
                {
                    event: "collection::find::toReactiveArray",
                    listener: this.#getCollectionFindToReactiveArrayListener(socket),
                },
                {
                    event: "collection::find::unsubscribe",
                    listener: this.#getCollectionFindUnsubscribeListener(socket),
                },
            ]).forEach(({event, listener}) => {
                // @ts-expect-error listener type mismatch
                socket.on(event, listener);
            });
        });
    }

    #getCollectionDisconnectListener (socket: Socket): EventListener<"disconnect"> {
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

    #getCollectionInitListener (socket: Socket): EventListener<"collection::init"> {
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

            socket.data = {collectionName};
        };
    }

    #getCollectionFindToArrayListener (socket: Socket): EventListener<"collection::find::toArray"> {
        return async ({query, options}, callback) => {
            const {collectionName} = socket.data as {collectionName: string};
            const collection = this.#collections.get(collectionName);

            if ("undefined" === typeof collection) {
                callback({error: "Collection not initialized"});

                return;
            }

            const documents = await collection.find(query, options).toArray();

            callback({data: documents});
        };
    }

    #getCollectionFindToReactiveArrayListener (socket: Socket)
        : EventListener<"collection::find::toReactiveArray"> {
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

    #getCollectionFindUnsubscribeListener (socket: Socket)
        : EventListener<"collection::find::unsubscribe"> {
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
