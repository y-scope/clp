
import styles from "./index.module.css";
import SearchButton from "./SearchButton/SearchButton";
import TimeRangeInput from "./TimeRangeInput";
import QueryInput from "./QueryInput";


/**
 * Renders controls for submitting queries.
 *
 * @return
 */
const SearchControls = () => {

    return (
        <div className={styles["searchControlsContainer"]}>
            <QueryInput />
            <TimeRangeInput/>
            <SearchButton/>
        </div>
    );
};


export default SearchControls;
