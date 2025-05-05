import {
    useEffect,
    useState,
} from "react";

import {MongoCursorSocket} from "./MongoCursorSocket.js";

/**
 * Custom hook to track a MongoDB cursor and return its data as a reactive array.
 *
 * @param find Function to create a MongoCursorSocket instance.
 * @param dependencies Array of dependencies for the query.
 * @return Reactive array of documents.
 */
const useCursor = (
    find: () => MongoCursorSocket,
    dependencies: object[] = []
): object[] => {
    const [data, setData] = useState<object[]>([]);

    useEffect(() => {
        const cursor = find();
        // Flag to ignore updates after cleanup.
        let ignore = false;
        console.log("Subscribing to cursor...");

        // Handler to set data updates from the server.
        const onDataUpdate = (data: object[]) => {
            if (false === ignore) {
                setData(data);
            }
        };

        const subscribed = cursor.subscribe(onDataUpdate);

        subscribed.catch((error) => {
            console.error("Error during subscription:", error);
        });

        return () => {
            ignore = true;
            // For a shortly lived cursor (ex. strict mode), the subscription may have not yet recieved
            // the queryID from the server, making it impossible to unsubscribe imediately (there is no queryID).
            // The promise allows unsubcription to happen when the subscription actually completes.
            subscribed
                .then(() => {
                    console.log("Unsubscribing from cursor...");
                    cursor.unsubscribe();
                })
        };
    }, [...dependencies]);

    console.log("useCursor data:", data);

    return data;
};

export {useCursor};
