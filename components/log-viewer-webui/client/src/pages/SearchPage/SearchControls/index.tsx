import styles from "./index.module.css";
import QueryInput from "./QueryInput";
import SearchButton from "./SearchButton/SearchButton";
import TimeRangeInput from "./TimeRangeInput";


/**
 * Renders controls for submitting queries.
 *
 * @return
 */
const SearchControls = () => {
    return (
        <div className={styles["searchControlsContainer"]}>
            <QueryInput/>
            <TimeRangeInput/>
            <SearchButton/>
        </div>
    );
};


export default SearchControls;
