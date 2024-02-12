/**
 * Class to keep track of MongoDB collections created for search jobs, ensuring all collections have
 * unique names.
 */
class SearchJobCollectionsManager {
    #collections;

    constructor() {
        this.#collections = new Map();
    }

    /**
     * Gets, or if it doesn't exist, creates a MongoDB collection named with the given job ID.
     *
     * @param {number} jobId
     * @returns {Mongo.Collection}
     */
    getOrCreateCollection(jobId) {
        const name = jobId.toString();
        if (undefined === this.#collections.get(name)) {
            this.#collections.set(name, new Mongo.Collection(name));
        }
        return this.#collections.get(name);
    }

    /**
     * Removes the MongoDB collection with the given job ID.
     *
     * @param {number} jobId
     */
    removeCollection(jobId) {
        this.#collections.delete(jobId.toString());
    }
}

export default SearchJobCollectionsManager;
