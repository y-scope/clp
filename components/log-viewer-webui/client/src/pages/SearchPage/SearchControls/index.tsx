import styles from "./index.module.css";
import QueryInput from "./QueryInput";
import SearchButton from "./SearchButton";
import TimeRangeInput from "./TimeRangeInput";
import Dataset from "./Dataset";


/**
 * Renders controls for submitting queries.
 *
 * @return
 */
const SearchControls = () => {
    // TODO: Replace with actual settings/config check for storage engine
    const storageEngine = "clp-s"; // This should come from settings/config
    const isClpSEngine = storageEngine === "clp-s";

    return (
        <div className={styles["searchControlsContainer"]}>
            {isClpSEngine && <Dataset/>}
            <QueryInput/>
            <TimeRangeInput/>
            <SearchButton/>
        </div>
    );
};


export default SearchControls;
