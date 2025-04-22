import {
    useEffect,
    useState,
} from "react";

import MongoCollectionReactiveCursor from "./MongoCollectionReactiveCursor.js";


/**
 * Custom hook to track a MongoDB cursor and return its data as an array.
 *
 * @param getCursor Function that returns a MongoReplicaCollectionReactiveCursor.
 * @param dependencies Array of dependencies for the effect.
 * @return The data from the cursor as an array.
 */
const useTracker = (
    getCursor: () => MongoCollectionReactiveCursor,
    dependencies: object[] = []
): unknown[] => {
    const [data, setData] = useState<unknown[]>([]);

    useEffect(() => {
        const cursor = getCursor();

        const handleData = (newData: unknown[]) => {
            setData(newData);
        };

        const unsubscribe = cursor.toReactiveArray({
            onData: handleData,
            onError: (error: unknown) => {
                console.error("Error fetching data:", error);
            },
        });

        return () => {
            unsubscribe();
        };
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, dependencies);

    return data;
};

export {useTracker};
