import {SearchOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import useSearchStore, {SEARCH_STATE_DEFAULT} from "../SearchState";
import styles from "./index.module.css";


/**
 * Renders a button to submit the search query.
 *
 * @return
 */
const SearchButton = () => {
    const queryString = useSearchStore((state) => state.queryString);

    const isQueryStringEmpty: boolean =
        queryString === SEARCH_STATE_DEFAULT.queryString;

    return (
        <Tooltip
            title={isQueryStringEmpty ?
                "Enter query to search" :
                ""}
        >
            <Button
                className={styles["gradientButton"] || ""}
                disabled={isQueryStringEmpty}
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
