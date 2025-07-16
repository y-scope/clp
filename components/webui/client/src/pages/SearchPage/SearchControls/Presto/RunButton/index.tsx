import {CaretRightOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import useSearchStore, {SEARCH_STATE_DEFAULT} from "../../../SearchState/index";
import styles from "../../SearchButton/SubmitButton/index.module.css";

/**
 * Renders a button to run the SQL query.
 *
 * @return
 */
const RunButton = () => {
    const queryString = useSearchStore((state) => state.queryString);

    const isQueryStringEmpty = queryString === SEARCH_STATE_DEFAULT.queryString;

    let tooltipTitle = "";
    if (isQueryStringEmpty) {
        tooltipTitle = "Enter SQL query to run";
    }

    return (
        <Tooltip title={tooltipTitle}>
            <Button
                className={styles["gradientButton"] || ""}
                icon={<CaretRightOutlined/>}
                size={"large"}
                type={"primary"}
            >
                Run
            </Button>
        </Tooltip>
    );
};

export default RunButton;
