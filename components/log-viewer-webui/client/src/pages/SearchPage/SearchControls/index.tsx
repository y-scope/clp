import styles from "./index.module.css";
import QueryInput from "./QueryInput";
import SearchButton from "./SearchButton";
import TimeRangeInput from "./TimeRangeInput";
import Dataset from "./Dataset";


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
    // TODO: Replace with actual settings/config check for storage engine
    const storageEngine = "clp-s"; // This should come from settings/config
    const isClpSEngine = storageEngine === "clp-s";

    return (
        <form onSubmit={handleSubmit}>
            <div className={styles["searchControlsContainer"]}>
                {isClpSEngine && <Dataset/>}
                <QueryInput/>
                <TimeRangeInput/>
                <SearchButton/>
            </div>
        </form>
    );
};


export default SearchControls;
