import { Socket } from "socket.io-client";

interface CursorConstructorArgs {
    socket: Socket;
    query: object;
    options: object;
}

class MongoReplicaCollectionCursor {
    socket: Socket;

    findQuery: object;

    findOptions: object;

    constructor ({socket, query, options}: CursorConstructorArgs) {
        this.socket = socket;

        this.findQuery = query;
        this.findOptions = options;
    }

    sort (sort: object): this {
        this.findOptions = {
            ...this.findOptions,
            sort: sort,
        };

        return this;
    }

    limit (number: number): this {
        this.findOptions = {
            ...this.findOptions,
            limit: number,
        };

        return this;
    }

    skip (offset:number): this {
        this.findOptions = {
            ...this.findOptions,
            skip: offset,
        };

        return this;
    }

    toArray () {
        return new Promise((resolve, reject) => {
            this.socket.emit("collection::find::toArray", {
                query: this.findQuery,
                options: this.findOptions,
            }, (response: { error?: any; data?: any }) => {
                if (response.error) {
                    return reject(response.error);
                }

                return resolve(response.data);
            });
        });
    }
}


export default MongoReplicaCollectionCursor;
