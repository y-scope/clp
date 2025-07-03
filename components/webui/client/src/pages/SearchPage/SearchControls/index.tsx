import styles from "./index.module.css";
import QueryInput from "./QueryInput";
import SearchButton from "./SearchButton/SearchButton";
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
                <QueryInput/>
                <TimeRangeInput/>
                <SearchButton/>
            </div>
        </form>
    );
};


export default SearchControls;
