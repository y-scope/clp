/* eslint-disable max-classes-per-file */
import {FastifyInstance} from "fastify";
import {
    MongoClient,
    MongoServerError
} from "mongodb";
import {Server} from "socket.io";


/**
 * Initialize the MongoDB replica set.
 *
 * @param {FastifyInstance} fastify
 * @return {Promise<void>}
 * @throws {Error} If the replica set initialization fails.
 */
const initializeReplicaSet = async (fastify: FastifyInstance): Promise<void> => {
    try {
        const directMongoClient = new MongoClient(
            "mongodb://localhost:27017",
            {replicaSet: "rs0", directConnection: true}
        );
        const response = await directMongoClient.db("admin").admin()
            .command({replSetInitiate: {}});

        fastify.log.info("Replica set initialized:", response);
    } catch (e) {
        if (e instanceof MongoServerError && "AlreadyInitialized" === e.codeName) {
            return;
        }
        throw new Error("Failed to initialize replica set", {cause: e});
    }
};


/**
 * TODO: Improve this? Think about security (other queries should not be able to kick others
 *  offline; maybe add a ref count then), performance, and collision chances.
 *
 * @param {string} query
 * @param options
 * @return {string}
 */
const getQueryHash = (query: object, options: object): string => JSON.stringify({query, options});


class MongoReplicaServerCollection {
    private count: number;

    private collection: any;

    private watchers: Map<string, any>;

    constructor(mongoDb: any, collectionName: string) {
        this.count = 0;
        this.collection = mongoDb.collection(collectionName);
        this.watchers = new Map();
    }

    /**
     * Increment the reference count;
     */
    refAdd () {
        this.count++;
    }

    /**
     * Decrement the reference count;
     */
    refRemove () {
        this.count--;
    }

    /**
     * Check if the collection is being referenced.
     *
     * @return {boolean}
     */
    isReferenced (): boolean {
        return 0 < this.count;
    }

    find (query: object, options: object) {
        return this.collection.find(query, options);
    }

    getWatcher (query: object, options: object) {
        const queryHash = getQueryHash(query, options);
        let watcher = this.watchers.get(queryHash);
        if ("undefined" === typeof watcher) {
            watcher = this.collection.watch({ $match: query });
            this.watchers.set(queryHash, watcher);
        }
        return { queryHash, watcher };
    }

    removeWatcher (queryHash: string) {
        if (this.watchers.has(queryHash)) {
            this.watchers.get(queryHash).close();
            this.watchers.delete(queryHash);
        }
    }
}

class MongoReplicaServer {
    private fastify: FastifyInstance;

    private collections: Map<string, MongoReplicaServerCollection>;

    private mongoDb: any;


    constructor ({fastify, mongoDb}: {fastify: FastifyInstance; mongoDb: any}) {
        console.log('hello world')
        this.fastify = fastify;
        this.collections = new Map();
        this.mongoDb = mongoDb;
        this.initializeSocketServer(fastify.server);
    }

    static async create ({
        fastify,
        dbName,
        mongoUri,
    }: {
        fastify: FastifyInstance;
        dbName: string;
        mongoUri: string;
    }): Promise<MongoReplicaServer> {
        const mongoDb = await MongoReplicaServer.initializeMongoClient({ mongoUri, dbName });

        return new MongoReplicaServer({fastify, mongoDb});
    }

    static async initializeMongoClient (
        {mongoUri, dbName}: {mongoUri: string; dbName: string}
    ): Promise<any> {
        const mongoClient = new MongoClient(mongoUri);
        try {
            await mongoClient.connect();

            return mongoClient.db(dbName);
        } catch (e) {
            throw new Error("MongoDB connection error", {cause: e});
        }
    }

    /* eslint-disable max-lines-per-function */
    private initializeSocketServer(httpServer: any) {
        const io = new Server(httpServer);
        console.log(io)

        io.on("connection", (socket) => {
            this.fastify.log.info(`Socket connected: ${socket.id}`);
            socket.on("disconnect", () => {
                this.fastify.log.info(`Socket disconnected: ${socket.id}`);
                const {collectionName} = socket.data;
                const collection = this.collections.get(collectionName);
                if ("undefined" !== typeof collection) {
                    collection.refRemove();
                    if (!collection.isReferenced()) {
                        this.fastify.log.info(`Collection ${collectionName} removed`);
                        this.collections.delete(collectionName);
                    }
                }
            });

            socket.on("collection::init", async ({collectionName}) => {
                this.fastify.log.info(`Collection name ${collectionName} requested`);

                let collection = this.collections.get(collectionName);
                if ("undefined" === typeof collection) {
                    collection = new MongoReplicaServerCollection(
                        this.mongoDb,
                        collectionName
                    );
                    this.collections.set(collectionName, collection);
                }
                collection.refAdd();

                socket.data = { collectionName };
            });

            socket.on("collection::find::toArray", async ({ query, options }, callback) => {
                const { collectionName } = socket.data;
                const collection = this.collections.get(collectionName);

                if ("undefined" === typeof collection) {
                    return callback({
                        error: "Collection not initialized",
                    });
                }

                const documents = await collection.find(query, options).toArray();

                return callback({ data: documents });
            }
            );

            socket.on("collection::find::toReactiveArray", async ({ query, options }, callback) => {
                this.fastify.log.info(`Collection name ${socket.data.collectionName} requested subscription`);
                const { collectionName } = socket.data;
                const collection = this.collections.get(collectionName);

                if ("undefined" === typeof collection) {
                    return callback({
                        error: "Collection not initialized",
                    });
                }

                const { queryHash, watcher } = collection.getWatcher(query, options);
                callback({ queryHash });
                watcher.on("change", async () => {
                    // FIXME: this should be debounced
                    socket.emit("collection::find::update", {
                        data: await collection.find(query, options).toArray(),
                    });
                });

                socket.emit("collection::find::update", {
                    data: await collection.find(query, options).toArray(),
                });
            }
            );

            socket.on("collection::find::unsubscribe", ({ queryHash }) => {
                this.fastify.log.info(`Collection name ${socket.data.collectionName} requested unsubscription`);

                const { collectionName } = socket.data;
                const collection = this.collections.get(collectionName);

                if ("undefined" === typeof collection) {
                    return;
                }

                collection.removeWatcher(queryHash);
            }
            );
        });
    }
}

/**
 * @typedef {object} MongoReplicaServerPluginOptions
 * @property {string} mongoUri MongoDB URI
 * @property {string} dbName Database name
 */

/**
 * MongoDB replica set plugin for Fastify.
 *
 * @param app
 * @param options
 * @param options.mongoUri
 * @param options.dbName
 */
// eslint-disable-next-line @stylistic/max-len
const MongoReplicaServerPlugin = async (app: FastifyInstance, options: {mongoUri: string; dbName: string}): Promise<void> => {
    console.log("registering")
    await initializeReplicaSet(app);

    app.decorate(
        "MongoReplicaServer",
        MongoReplicaServer.create({
            fastify: app,
            mongoUri: options.mongoUri,
            dbName: options.dbName,
        })
    );
};

export default MongoReplicaServerPlugin;
