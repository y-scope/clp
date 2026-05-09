import {InputNumber, Space, Typography} from "antd";

import {SETTINGS_MAX_SEARCH_RESULTS} from "../../../../config";
import useSearchStore from "../../SearchState";


/**
 * Renders an input for setting the maximum number of search results per query.
 *
 * @return
 */
const MaxResultsInput = () => {
    const maxNumResults = useSearchStore((state) => state.maxNumResults);
    const updateMaxNumResults = useSearchStore((state) => state.updateMaxNumResults);

    return (
        <Space.Compact>
            <Typography.Text
                style={{
                    alignSelf: "center",
                    marginRight: 4,
                    whiteSpace: "nowrap",
                }}
            >
                {"Limit"}
            </Typography.Text>
            <InputNumber
                changeOnBlur={true}
                max={SETTINGS_MAX_SEARCH_RESULTS}
                min={1}
                size={"middle"}
                style={{width: 100}}
                title={"Max Results"}
                value={maxNumResults}
                onChange={(value) => {
                    if (null !== value) {
                        updateMaxNumResults(value);
                    }
                }}/>
        </Space.Compact>
    );
};

export default MaxResultsInput;
