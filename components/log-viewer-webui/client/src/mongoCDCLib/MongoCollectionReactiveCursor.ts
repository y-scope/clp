import {Socket} from "socket.io-client";

import MongoCollectionCursor from "./MongoCollectionCursor.js";


interface Listener {
    onData: (data: Document[]) => void;
    onError: (error: Error) => void;
}

interface ReactiveArrayCallback {
    onData: (data: Document[]) => void;
    onError: (error: Error) => void;
}

/**
 * Represents a reactive cursor for querying a MongoDB-like collection over a socket connection.
 * This class extends MongoCollectionCursor to provide real-time updates.
 *
 * @class MongoCollectionReactiveCursor
 * @augments MongoCollectionCursor
 */
class MongoCollectionReactiveCursor extends MongoCollectionCursor {
    // The listener for data and error events.
    private listener: Listener | null = null;

    // The unique identifier for the query.
    private queryId: number | null = null;

    /**
     * Creates an instance of MongoCollectionReactiveCursor.
     *
     * @param props The constructor properties.
     * @param props.socket The socket connection to the server.
     * @param props.query The query object to filter results.
     * @param props.options The options for the query (e.g., sort, limit).
     */
    constructor (props: {socket: Socket; query: object; options: object}) {
        super(props);

        this.socket.on(
            "collection::find::update",
            (response: {error?: Error; data?: Document[]; queryId: number}) => {
                if (this.queryId === response.queryId) {
                    if (response.error) {
                        return this.listener?.onError(response.error);
                    }

                    return this.listener?.onData(response.data ?? []);
                }

                return null;
            }
        );
    }

    /**
     * Subscribe to the collection for real-time updates.
     *
     * @param callback
     * @return The cleanup function.
     */
    toReactiveArray (callback: ReactiveArrayCallback): () => void {
        const unsubscribedInfo : {queryId: number | null} = {queryId: null};

        this.socket.emit("collection::find::toReactiveArray", {
            query: this.findQuery,
            options: this.findOptions,
        }, (response: {error?: Error; queryId?: number}) => {
            if (response.error) {
                callback.onError(response.error);

                return;
            }

            if ("undefined" !== typeof response.queryId) {
                unsubscribedInfo.queryId = response.queryId;
                this.queryId = response.queryId;
            }

            this.listener = callback;
        });

        return () => {
            if (null === unsubscribedInfo.queryId) {
                return;
            }
            this.socket.emit("collection::find::unsubscribe", {
                queryId: unsubscribedInfo.queryId,
            });

            this.socket.off("collection::find::update");
        };
    }
}

export default MongoCollectionReactiveCursor;
