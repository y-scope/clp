import {CaretRightOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import useSearchStore from "../../../SearchState/index";


/**
 * Renders a button to run the SQL query.
 *
 * @return
 */
const RunButton = () => {
    const queryString = useSearchStore((state) => state.queryString);

    const isQueryStringEmpty = queryString === "";
    const tooltipTitle = isQueryStringEmpty ? "Enter SQL query to run" : "";

    return (
        <Tooltip title={tooltipTitle}>
            <Button
                icon={<CaretRightOutlined/>}
                color="green"
                variant ="solid"
                size={"large"}
                disabled={isQueryStringEmpty}
            >
                Run
            </Button>
        </Tooltip>
    );
};

export default RunButton;
