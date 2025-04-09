import {
    useEffect,
    useState,
} from "react";

/**
 * Custom hook to track a MongoDB cursor and return its data as an array.
 *
 * @param {Function} getCursor Function that returns a MongoReplicaCollectionReactiveCursor.
 * @param {object[]} dependencies Array of dependencies for the effect.
 * @return {object[]} The data from the cursor as an array.
 */
const useTracker = (
    getCursor: Function,
    dependencies: object[] = []
): any[] => {
    const [data, setData] = useState<any[]>([]);

    useEffect(() => {
        const cursor = getCursor();

        const handleData = (newData: any[]) => setData(newData);

        const unsubscribe = cursor.toReactiveArray({
            onData: handleData,
            onError: (error: any) => {
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
