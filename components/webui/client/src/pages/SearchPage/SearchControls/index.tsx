import {
    CLP_QUERY_ENGINES,
    CLP_STORAGE_ENGINES,
    SETTINGS_QUERY_ENGINE,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";
import Dataset from "./Dataset";
import styles from "./index.module.css";
import SqlInterfaceButton from "./Presto/SqlInterfaceButton";
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
    /* eslint-disable-next-line no-warning-comments */
    // TODO: Remove flag and related logic when the new guide UI is fully implemented.
    const isGuidedEnabled = "true" === import.meta.env.VITE_GUIDED_DEV;

    return (
        <form onSubmit={handleSubmit}>
            {SETTINGS_QUERY_ENGINE !== CLP_QUERY_ENGINES.PRESTO ?
                (
                    <div className={styles["searchControlsContainer"]}>
                        <div className={styles["inputsAndButtonRow"]}>
                            {CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE && <Dataset/>}
                            <QueryInput/>
                            <TimeRangeInput/>
                            <SearchButton/>
                        </div>
                        <div className={styles["status"]}>
                            <QueryStatus/>
                        </div>
                    </div>
                ) :
                (
                    <div className={styles["searchControlsContainer"]}>
                        <SqlQueryInput/>
                        <div className={styles["statusAndButtonsRow"]}>
                            <div className={styles["status"]}>
                                <QueryStatus/>
                            </div>
                            <div className={styles["buttons"]}>
                                {isGuidedEnabled && <SqlInterfaceButton/>}
                                <SqlSearchButton/>
                            </div>
                        </div>
                    </div>
                )}
        </form>
    );
};

export default SearchControls;
