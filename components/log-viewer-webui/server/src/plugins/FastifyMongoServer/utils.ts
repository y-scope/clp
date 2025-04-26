import {
    Db,
    MongoClient,
} from "mongodb";


interface ChangeStreamQuery {
    [key: string]: unknown;
}

/**
 * Converts a MongoDB find query into a change stream query.
 *
 * @param findQuery
 * @return A change stream query object.
 * The keys of the find query are prefixed with "fullDocument." to match the change stream format.
 */
const convertFindToChangeStreamQuery = function (findQuery: Record<string, unknown>) {
    const changeStreamQuery: ChangeStreamQuery = {};
    for (const key in findQuery) {
        if (Object.hasOwn(findQuery, key)) {
            changeStreamQuery[`fullDocument.${key}`] = findQuery[key];
        }
    }

    return changeStreamQuery;
};

/**
 * // eslint-disable-next-line no-warning-comments
 * TODO: Improve this? Think about security (other queries should not be able to kick others
 *  offline; maybe add a ref count then), performance, and collision chances.
 *
 * Generates a unique hash for a given query and options.
 * This hash is used to identify and manage change streams for specific queries.
 *
 * @param collectionName
 * @param query The query object to be hashed.
 * @param options The options object to be hashed.
 * @return A string representing the unique hash for the query and options.
 */
const getQueryHash = function (
    collectionName: string,
    query: object,
    options: object
): string {
    return JSON.stringify({collectionName, query, options});
};

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

export {convertFindToChangeStreamQuery};
export {getQueryHash};
export {initializeMongoClient};
