import {
    Select,
    SelectProps,
} from "antd";
import {useQuery} from "@tanstack/react-query";

import useSearchStore from "../../../../SearchState";
import usePrestoSearchState from "../../../../SearchState/Presto";
import {SEARCH_UI_STATE} from "../../../../SearchState/typings";
import {fetchTimestampColumns} from "./sql";


/**
 * Renders a timestamp key selector component.
 *
 * @param selectProps
 * @return
 */
const TimestampKeySelect = (selectProps: SelectProps<string>) => {
    const dataset = useSearchStore((state) => state.selectDataset);
    const timestampKey = usePrestoSearchState((state) => state.timestampKey);
    const updateTimestampKey = usePrestoSearchState((state) => state.updateTimestampKey);
    const searchUiState = useSearchStore((state) => state.searchUiState);

    // Access cached timestamp keys fetched by useTimestampKeyInit hook in GuidedControls.
    const {data: timestampKeys} = useQuery({
        queryKey: [
            "timestampColumns",
            dataset,
        ],
        queryFn: () => (dataset ?
            fetchTimestampColumns(dataset) :
            []),
        enabled: null !== dataset,
    });

    const handleTimestampKeyChange = (value: string) => {
        updateTimestampKey(value);
    };

    return (
        <Select<string>
            options={(timestampKeys || []).map((option) => ({label: option, value: option}))}
            value={timestampKey}
            disabled={
                null === dataset ||
                searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                searchUiState === SEARCH_UI_STATE.QUERYING
            }
            onChange={handleTimestampKeyChange}
            {...selectProps}/>
    );
};

export default TimestampKeySelect;
