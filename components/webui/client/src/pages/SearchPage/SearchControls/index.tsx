import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";
import {settings} from "../../../settings";
import SqlEditor from "../../../components/SqlEditor";
import Dataset from "./Dataset";
import styles from "./index.module.css";
import QueryInput from "./QueryInput";
import SearchButton from "./SearchButton";
import TimeRangeInput from "./TimeRangeInput";
import {useState} from "react";


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
    const isPrestoEngine = settings.ClpQueryEngine === "presto";

    return (
        <form onSubmit={handleSubmit}>
            <div className={styles["searchControlsContainer"]}>
                {isPrestoEngine ? (
                    <SqlEditor/>
                ) : (
                    <>
                        {CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE && <Dataset/>}
                        <QueryInput/>
                        <TimeRangeInput/>
                        <SearchButton/>
                    </>
                )}
            </div>
        </form>
    );
};


export default SearchControls;
