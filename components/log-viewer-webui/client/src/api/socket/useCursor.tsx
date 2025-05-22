import {
    DependencyList,
    useEffect,
    useState,
} from "react";

import {MongoCursorSocket} from "./MongoCursorSocket.js";


/**
 * Custom hook which returns a real-time reactive array of documents from a `MongoCursorSocket`.
 *
 * @param query Function which returns a `MongoCursorSocket` instance.
 * @param dependencies Array of dependencies for the query.
 * @return Reactive array.
 */
const useCursor = (
    query: () => MongoCursorSocket | null,
    dependencies: DependencyList = []
): object[] => {
    const [data, setData] = useState<object[]>([]);


    useEffect(() => {
        const cursor = query();

        if (!cursor) {
            return;
        }

        // Flag to ignore updates after unmounting.
        let ignore = false;
        console.log("Subscribing to cursor");

        // Handler to set data updates from the server.
        const onDataUpdate = (dataUpdate: object[]) => {
            if (false === ignore) {
                setData(dataUpdate);
            }
        };

        const subscribed = cursor.subscribe(onDataUpdate);

        subscribed.catch((error: unknown) => {
            console.error("Error during subscription:", error);
        });

        return () => {
            ignore = true;

            // For a shortly lived cursor (ex. strict mode), the subscription may have not yet
            // recieved the queryID from the server, making it impossible to unsubscribe
            // immediately (there is no queryID). The subscribed promise allows unsubcription
            // to happen when the subscription actually completes.
            subscribed
                .then(() => {
                    // Unsubscribe will not run if the subscription failed since the promise was
                    // rejected.
                    console.log("Unsubscribing from cursor");
                    cursor.unsubscribe();
                })
                .catch((error: unknown) => {
                    console.error("Error during unsubscription:", error);
                });
        };
    // eslint-disable-next-line react-hooks/exhaustive-deps
    }, dependencies);

    return data;
};

export {useCursor};
