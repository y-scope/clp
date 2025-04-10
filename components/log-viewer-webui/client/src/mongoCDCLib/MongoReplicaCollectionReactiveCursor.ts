import {Socket} from "socket.io-client";

import MongoReplicaCollectionCursor from "./MongoReplicaCollectionCursor.js";


interface Listener {
    onData: (data: Document[]) => void;
    onError: (error: any) => void;
}

interface ReactiveArrayCallback {
    onData: (data: Document[]) => void;
    onError: (error: any) => void;
}

class MongoReplicaCollectionReactiveCursor extends MongoReplicaCollectionCursor {
    /**
     * @type {Listener}
     */
    private listener: Listener | null = null;

    constructor (props: {socket: Socket; query: object; options: object}) {
        super(props);

        this.socket.on("collection::find::update", (response: {error?: any; data?: Document[]}) => {
            if (response.error) {
                return this.listener?.onError(response.error);
            }

            return this.listener?.onData(response.data ?? []);
        });
    }

    /**
     * Subscribe to the collection
     *
     * @param callback
     * @return The cleanup function.
     */
    toReactiveArray (callback: ReactiveArrayCallback): () => void {
        console.log("toReactiveArray");
        let queryHash: string | undefined;
        this.socket.emit("collection::find::toReactiveArray", {
            query: this.findQuery,
            options: this.findOptions,
        }, (response: {error?: any; queryHash?: string}) => {
            if (response.error) {
                callback.onError(response.error);

                return;
            }

            ({queryHash} = response);

            this.listener = callback;
        });

        return () => {
            // TODO: the server should also rely on some heartbeat from the client
            //  for deciding when to free the resources.
            this.socket.emit("collection::find::unsubscribe", {
                queryHash: queryHash,
            });

            this.socket.off("collection::find::update");
        };
    }
}

export default MongoReplicaCollectionReactiveCursor;
