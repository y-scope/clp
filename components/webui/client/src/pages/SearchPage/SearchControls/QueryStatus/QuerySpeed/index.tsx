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
    const cachedDatasets = useSearchStore((state) => state.cachedDatasets);

    const {data = SPEED_DEFAULT} = useQuery({
        queryKey: [
            "speed",
            searchJobId,
            cachedDatasets,
        ],
        queryFn: async () => {
            if (null === searchJobId) {
                return SPEED_DEFAULT;
            }
            const datasetName = cachedDatasets[0] ?? "";
            const {bytes, duration} = await fetchQuerySpeed(datasetName, searchJobId);

            if (null === bytes || null === duration) {
                return SPEED_DEFAULT;
            }

            return {latency: duration, speed: bytes / duration};
        },
        enabled: SEARCH_UI_STATE.DONE === searchUiState,
        refetchInterval: false,
    });
    const {latency, speed} = data;
    return (
        <Text type={"secondary"}>
            {` in ${latency.toFixed(3)} seconds (${formatSizeInBytes(speed)}/s)`}
        </Text>
    );
};

export default QuerySpeed;
