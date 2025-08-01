import {
    CLP_QUERY_ENGINES,
    CLP_STORAGE_ENGINES,
    SETTINGS_QUERY_ENGINE,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";
import Dataset from "./Dataset";
import styles from "./index.module.css";
import RunButton from "./Presto/RunButton";
import SqlQueryInput from "./Presto/SqlQueryInput";
import QueryInput from "./QueryInput";
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
 * Renders controls for submitting queries.
 *
 * @return
 */
const SearchControls = () => {
    return (
        <form onSubmit={handleSubmit}>
            <div className={styles["searchControlsContainer"]}>
                {SETTINGS_QUERY_ENGINE === CLP_QUERY_ENGINES.NATIVE ?
                    (
                        <>
                            {CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE && <Dataset/>}
                            <QueryInput/>
                            <TimeRangeInput/>
                            <SearchButton/>
                        </>
                    ) :
                    (
                        <>
                            <SqlQueryInput/>
                            <RunButton/>
                        </>
                    )}
            </div>
        </form>
    );
};


export default SearchControls;
