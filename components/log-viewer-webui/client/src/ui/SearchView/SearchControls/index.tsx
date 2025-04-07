import {useContext} from "react";

import {Input} from "antd";

import {SearchContext} from "../SearchContext";
import styles from "./index.module.css";
import SearchButton from "./SearchButton";
import TimeRangeInput from "./TimeRangeInput";


/**
 * Renders controls for submitting queries.
 *
 * @return
 */
const SearchControls = () => {
    const {queryString, setQueryString} = useContext(SearchContext);

    return (
        <div className={styles["search-controls-container"]}>
            <Input
                placeholder={"Enter your query"}
                size={"large"}
                value={queryString}
                onChange={(e) => {
                    setQueryString(e.target.value);
                }}/>
            <TimeRangeInput/>
            <SearchButton/>
        </div>
    );
};

export default SearchControls;
