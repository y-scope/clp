/**
 * Class to keep track of MongoDB collections created for search jobs, ensuring all collections have
 * unique names.
 */
class SearchJobCollectionsManager {
    #collections;

    constructor () {
        // NOTE: do not remove inserted collections because we need to check for duplicated
        // Collection before constructing a new one, or Meteor would complain.
        // TODO: revisit: memory leak?
        this.#collections = new Map();
    }

    /**
     * Gets, or if it doesn't exist, creates a MongoDB collection named with the given job ID.
     *
     * @param {number} jobId
     * @returns {Mongo.Collection}
     */
    getOrCreateCollection (jobId) {
        const name = jobId.toString();
        if (undefined === this.#collections.get(name)) {
            this.#collections.set(name, new Mongo.Collection(name));
        }
        return this.#collections.get(name);
    }
}

export default SearchJobCollectionsManager;
