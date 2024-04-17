const ERROR_NAME_COLLECTION_DROPPED = "collection-dropped";

/**
 * Class to keep track of MongoDB collections created for search jobs, ensuring all collections have
 * unique names.
 */
class SearchJobCollectionsManager {
    #collections;

    constructor () {
        this.#collections = new Map();
    }

    /**
     * Gets, or if it doesn't exist, creates a MongoDB collection named with the given job ID.
     *
     * @param {number} jobId
     * @return {Mongo.Collection}
     * @throws {Meteor.Error} with ERROR_NAME_COLLECTION_DROPPED if the collection was already
     * dropped.
     */
    getOrCreateCollection (jobId) {
        const name = jobId.toString();
        if ("undefined" === typeof this.#collections.get(name)) {
            this.#collections.set(name, new Mongo.Collection(name));
        } else if (null === this.#collections.get(name)) {
            throw new Meteor.Error(
                ERROR_NAME_COLLECTION_DROPPED,
                `Collection ${name} has been dropped.`
            );
        }

        return this.#collections.get(name);
    }

    /**
     * Drops the MongoDB collection with the given job ID.
     *
     * @param {number} jobId
     */
    async dropCollection (jobId) {
        const name = jobId.toString();
        const collection = this.#collections.get(name);

        await collection.dropCollectionAsync();
        this.#collections.set(name, null);
    }
}

export default SearchJobCollectionsManager;
export {ERROR_NAME_COLLECTION_DROPPED};
