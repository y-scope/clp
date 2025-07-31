import {useCallback} from "react";

import {CaretRightOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import useSearchStore from "../../../SearchState/index";
import {handlePrestoQuerySubmit} from "../presto-search-requests";


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

    const handleClick = useCallback(() => {
        handlePrestoQuerySubmit({queryString});
    }, [queryString]);

    return (
        <Tooltip title={tooltipTitle}>
            <Button
                color={"green"}
                disabled={isQueryStringEmpty}
                icon={<CaretRightOutlined/>}
                size={"large"}
                variant={"solid"}
                onClick={handleClick}
            >
                Run
            </Button>
        </Tooltip>
    );
};

export default RunButton;
