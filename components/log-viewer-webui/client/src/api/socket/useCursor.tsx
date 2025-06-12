import {
    DependencyList,
    useEffect,
    useState,
} from "react";

import {Nullable} from "../../typings/common";
import {MongoSocketCursor} from "./MongoSocketCursor.js";


/**
 * Custom hook which returns a real-time reactive array of documents from a `MongoSocketCursor`.
 *
 * @template T The document type returned by the cursor.
 * @param query Function which returns a `MongoSocketCursor` instance or null.
 * @param dependencies Array of dependencies for the query.
 * @return
 * - If `query` returns a `MongoSocketCursor` instance, then hook returns null while
 * the subscription is pending, and a reactive array of documents when the subscription is ready.
 * - If `query` returns null, then the hook also returns null.
 */
const useCursor = <T = object>(
    query: () => Nullable<MongoSocketCursor>,
    dependencies: DependencyList = []
): Nullable<T[]> => {
    const [data, setData] = useState<Nullable<T[]>>(null);

    useEffect(() => {
        const cursor = query();

        if (null === cursor) {
            setData(null);

            return () => {
            };
        }

        // Flag to ignore updates after unmounting.
        let ignore = false;

        // Handler to set data updates from the server.
        const onDataUpdate = (dataUpdate: object[]) => {
            if (false === ignore) {
                setData(dataUpdate as T[]);
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
