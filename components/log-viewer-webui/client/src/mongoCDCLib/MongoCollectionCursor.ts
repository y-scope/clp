import {Socket} from "socket.io-client";


interface CursorConstructorArgs {
    socket: Socket;
    query: object;
    options: object;
}

/**
 * Represents a cursor for querying a MongoDB collection over a socket connection.
 *
 * @class MongoCollectionCursor
 */
class MongoCollectionCursor {
    socket: Socket;

    findQuery: object;

    findOptions: object;

    /**
     * Creates an instance of MongoCollectionCursor.
     *
     * @param args The constructor arguments.
     * @param args.socket The socket connection to the server.
     * @param args.query The query object to filter results.
     * @param args.options The options for the query (e.g., sort, limit).
     */
    constructor ({socket, query, options}: CursorConstructorArgs) {
        this.socket = socket;

        this.findQuery = query;
        this.findOptions = options;
    }

    /**
     * Adds a sort option to the query.
     *
     * @param sort The sort criteria.
     * @return The current instance for method chaining.
     */
    sort (sort: object): this {
        this.findOptions = {
            ...this.findOptions,
            sort: sort,
        };

        return this;
    }

    /**
     * Sets a limit on the number of results returned.
     *
     * @param number The maximum number of results to return.
     * @return The current instance for method chaining.
     */
    limit (number: number): this {
        this.findOptions = {
            ...this.findOptions,
            limit: number,
        };

        return this;
    }

    /**
     * Skips a specified number of results.
     *
     * @param offset The number of results to skip.
     * @return The current instance for method chaining.
     */
    skip (offset:number): this {
        this.findOptions = {
            ...this.findOptions,
            skip: offset,
        };

        return this;
    }

    /**
     * Executes the query and returns the results as an array.
     *
     * @return A promise that resolves with the array of documents or rejects with an error.
     */
    toArray () {
        return new Promise((resolve, reject) => {
            this.socket.emit("collection::find::toArray", {
                query: this.findQuery,
                options: this.findOptions,
            }, (response: {error?: Error; data?: Document[]}) => {
                if (response.error) {
                    reject(response.error);

                    return;
                }

                resolve(response.data);
            });
        });
    }
}


export default MongoCollectionCursor;
