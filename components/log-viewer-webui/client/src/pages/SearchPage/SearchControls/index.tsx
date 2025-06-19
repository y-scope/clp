import styles from "./index.module.css";
import QueryInput from "./QueryInput";
import SearchButton from "./SearchButton";
import TimeRangeInput from "./TimeRangeInput";
import Dataset from "./Dataset";
import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";


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
                {CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE && <Dataset/>}
                <QueryInput/>
                <TimeRangeInput/>
                <SearchButton/>
            </div>
        </form>
    );
};


export default SearchControls;
