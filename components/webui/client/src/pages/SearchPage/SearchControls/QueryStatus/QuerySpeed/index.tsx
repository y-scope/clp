import {useQuery} from "@tanstack/react-query";
import {Typography} from "antd";

import {formatSizeInBytes} from "../../../../IngestPage/Jobs/units";
import useSearchStore from "../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import {fetchQuerySpeed} from "./utils";


const {Text} = Typography;
const SPEED_DEFAULT = {latency: 0, speed: 0};

/**
 * Displays the query latency and speed.
 *
 * @return
 */
const QuerySpeed = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const searchJobId = useSearchStore((state) => state.searchJobId);
    const cachedDataset = useSearchStore((state) => state.cachedDataset);

    const {data = SPEED_DEFAULT} = useQuery({
        queryKey: [
            "speed",
            searchJobId,
            cachedDataset,
        ],
        queryFn: async () => {
            const {bytes, duration} = await fetchQuerySpeed(cachedDataset ?? "", searchJobId ?? "");
            return {latency: duration, speed: bytes / duration};
        },
        enabled: SEARCH_UI_STATE.DONE === searchUiState,
    });
    const {latency, speed} = data;
    return (
        <Text type={"secondary"}>
            {`Query took ${latency.toFixed(3)} seconds (${formatSizeInBytes(speed)}/s).`}
        </Text>
    );
};

export default QuerySpeed;
