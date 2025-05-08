import QueryBox from "../../../components/QueryBox";
import useSearchStore from "../SearchState";
import styles from "./index.module.css";
import SearchButton from "./SearchButton";
import TimeRangeInput from "./TimeRangeInput";


/**
 * Renders controls for submitting queries.
 *
 * @return
 */
const SearchControls = () => {
    const queryString = useSearchStore((state) => state.queryString);
    const updateQueryString = useSearchStore((state) => state.updateQueryString);

    return (
        <div className={styles["searchControlsContainer"]}>
            <QueryBox
                placeholder={"Enter your query"}
                progress={null}
                size={"large"}
                value={queryString}
                onChange={(e) => {
                    updateQueryString(e.target.value);
                }}/>
            <TimeRangeInput/>
            <SearchButton/>
        </div>
    );
};


export default SearchControls;
