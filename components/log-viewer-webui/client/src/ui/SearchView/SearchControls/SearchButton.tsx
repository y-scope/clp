import {useContext} from "react";

import {SearchOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import {
    SEARCH_CONTEXT_DEFAULT,
    SearchContext,
} from "../SearchContext";
import styles from "./index.module.css";


/**
 * Renders a button to submit the search query.
 *
 * @return
 */
const SearchButton = () => {
    const {queryString} = useContext(SearchContext);

    const isQueryStringValid: boolean = queryString === SEARCH_CONTEXT_DEFAULT.queryString;

    return (
        <Tooltip
            title={isQueryStringValid ?
                "Enter query to search" :
                ""}
        >
            <Button
                className={styles["gradient-button"] || ""}
                disabled={false === isQueryStringValid}
                icon={<SearchOutlined/>}
                size={"large"}
                type={"primary"}
            >
                Search
            </Button>
        </Tooltip>
    );
};

export default SearchButton;
