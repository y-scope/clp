import {useCallback} from "react";

import {
    InputNumber,
    Space,
    Typography,
} from "antd";

import {SETTINGS_MAX_SEARCH_RESULTS} from "../../../../config";
import useSearchStore from "../../SearchState";
import styles from "./index.module.css";


/**
 * Renders an input for setting the maximum number of search results per query.
 *
 * @return
 */
const MaxResultsInput = () => {
    const maxNumResults = useSearchStore((state) => state.maxNumResults);

    const handleChange = useCallback((value: number | null) => {
        if (null !== value) {
            const {updateMaxNumResults} = useSearchStore.getState();
            updateMaxNumResults(value);
        }
    }, []);

    return (
        <Space.Compact>
            <Typography.Text className={styles["label"] ?? ""}>
                {"Limit"}
            </Typography.Text>
            <InputNumber
                changeOnBlur={true}
                className={styles["input"] ?? ""}
                max={SETTINGS_MAX_SEARCH_RESULTS}
                min={1}
                size={"middle"}
                title={"Max Results"}
                value={maxNumResults}
                onChange={handleChange}/>
        </Space.Compact>
    );
};

export default MaxResultsInput;
