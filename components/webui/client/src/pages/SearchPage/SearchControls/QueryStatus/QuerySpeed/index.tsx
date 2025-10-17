import {
    useEffect,
    useState,
} from "react";

import {Typography} from "antd";

import useSearchStore from "../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import {fetchQuerySpeed} from "./utils";


const {Text} = Typography;

const MEGABYTES_IN_BYTES = 1_000_000;

/**
 * Displays the query latency and speed.
 *
 * @return
 */
const QuerySpeed = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const [latency, setLatency] = useState<number>(0);
    const [speed, setSpeed] = useState<number>(0);
    useEffect(() => {
        if (searchUiState !== SEARCH_UI_STATE.DONE) {
            return;
        }
        (async () => {
            const {cachedDataset, searchJobId} = useSearchStore.getState();
            const {bytes, duration} = await fetchQuerySpeed(cachedDataset ?? "", searchJobId ?? "");
            setLatency(duration);
            setSpeed(bytes / MEGABYTES_IN_BYTES / duration);
        })().catch(console.error);
    }, [searchUiState]);

    return (
        <Text type={"secondary"}>
            {`Query took ${latency.toFixed(3)} seconds (${speed.toFixed(3)} MB/s).`}
        </Text>
    );
};

export default QuerySpeed;
