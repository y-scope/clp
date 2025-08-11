import {useCallback} from "react";

import SqlEditor from "../../../../../components/SqlEditor";
import useSearchStore from "../../../SearchState/index";
import styles from "./index.module.css";


/**
 * Renders SQL query input.
 *
 * @return
 */
const SqlQueryInput = () => {
    const handleChange = useCallback((value: string | undefined) => {
        const {updateQueryString} = useSearchStore.getState();
        updateQueryString(value || "");
    }, []);

    return (
        <div className={styles["input"] || ""}>
            <SqlEditor onChange={handleChange}/>
        </div>
    );
};

export default SqlQueryInput;
