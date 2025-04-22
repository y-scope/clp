import {
    Db,
    MongoClient,
} from "mongodb";


/**
 * Initializes the MongoDB client and connects to the specified database.
 *
 * @param database The name of the database to connect to.
 * @param host The host of the MongoDB server.
 * @param port The port of the MongoDB server.
 * @return A promise that resolves to the MongoDB database instance.
 */
const initializeMongoClient = async function (
    database: string,
    host: string,
    port: string
): Promise<Db> {
    const mongoUri = `mongodb://${host}:${port}`;
    const mongoClient = new MongoClient(mongoUri);
    try {
        await mongoClient.connect();

        return mongoClient.db(database);
    } catch (e) {
        throw new Error("MongoDB connection error", {cause: e});
    }
};

export {initializeMongoClient};
