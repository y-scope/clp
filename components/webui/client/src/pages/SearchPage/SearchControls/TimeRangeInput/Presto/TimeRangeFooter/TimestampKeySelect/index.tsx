import {useQuery} from "@tanstack/react-query";
import {
    Select,
    SelectProps,
} from "antd";

import useSearchStore from "../../../../../SearchState";
import usePrestoSearchState from "../../../../../SearchState/Presto";
import {fetchTimestampColumns} from "../../../../../SearchState/Presto/useTimestampKeyInit/sql";
import {SEARCH_UI_STATE} from "../../../../../SearchState/typings";


/**
 * Renders a timestamp key selector component.
 *
 * @param selectProps
 * @return
 */
const TimestampKeySelect = (selectProps: SelectProps<string>) => {
    const selectDatasets = useSearchStore((state) => state.selectDatasets);
    const timestampKey = usePrestoSearchState((state) => state.timestampKey);
    const updateTimestampKey = usePrestoSearchState((state) => state.updateTimestampKey);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const dataset = 0 < selectDatasets.length ?
        selectDatasets[0] :
        null;

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
