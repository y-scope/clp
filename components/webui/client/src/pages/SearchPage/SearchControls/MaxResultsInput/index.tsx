import {InputNumber} from "antd";

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
        <InputNumber
            changeOnBlur={true}
            max={SETTINGS_MAX_SEARCH_RESULTS}
            min={1}
            size={"middle"}
            style={{width: 150}}
            title={"Max Results"}
            value={maxNumResults}
            addonBefore={"Limit"}
            onChange={(value) => {
                if (null !== value) {
                    updateMaxNumResults(value);
                }
            }}/>
    );
};

export default MaxResultsInput;
