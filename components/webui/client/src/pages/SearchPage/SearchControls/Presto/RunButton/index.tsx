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

    const isQueryStringEmpty = "" === queryString;
    const tooltipTitle = isQueryStringEmpty ?
        "Enter SQL query to run" :
        "";

    return (
        <Tooltip title={tooltipTitle}>
            <Button
                color={"green"}
                disabled={isQueryStringEmpty}
                icon={<CaretRightOutlined/>}
                size={"large"}
                variant={"solid"}
            >
                Run
            </Button>
        </Tooltip>
    );
};

export default RunButton;
