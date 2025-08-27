import {
    CLP_QUERY_ENGINES,
    CLP_STORAGE_ENGINES,
    SETTINGS_QUERY_ENGINE,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";
import Dataset from "./Dataset";
import styles from "./index.module.css";
import SqlQueryInput from "./Presto/SqlQueryInput";
import SqlSearchButton from "./Presto/SqlSearchButton";
import QueryInput from "./QueryInput";
import QueryStatus from "./QueryStatus";
import SearchButton from "./SearchButton";
import TimeRangeInput from "./TimeRangeInput";


/**
 * Prevents the default behavior to avoid page reload when submitting query.
 *
 * @param ev
 */
const handleSubmit = (ev: React.FormEvent<HTMLFormElement>) => {
    ev.preventDefault();
};

/**
 * Renders controls for submitting queries and the query status.
 *
 * @return
 */
const SearchControls = () => {
    return (
        <form onSubmit={handleSubmit}>
            {SETTINGS_QUERY_ENGINE !== CLP_QUERY_ENGINES.PRESTO ?
                (
                    <div>
                        <div className={styles["searchControlsContainer"]}>
                            {CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE && <Dataset/>}
                            <QueryInput/>
                            <TimeRangeInput/>
                            <SearchButton/>
                        </div>
                        <QueryStatus/>
                    </div>
                ) :
                (

                    <div className={styles["prestoSearchControlsContainer"]}>
                        <SqlQueryInput/>
                        <div className={styles["buttonAndStatusRow"]}>
                            <div style={{ alignSelf: "flex-start" }}>
                                <QueryStatus/>
                            </div>
                            <div style={{ flex: 1 }} />
                            <div style={{ alignSelf: "flex-end" }}>
                                <SqlSearchButton/>
                            </div>
                        </div>
                    </div>
                )}
        </form>
    );
};


export default SearchControls;
