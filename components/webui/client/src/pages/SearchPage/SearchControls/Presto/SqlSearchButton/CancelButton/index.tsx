import {useCallback} from "react";

import {CloseOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import useSearchStore from "../../../../SearchState/index";
import usePrestoSearchState from "../../../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../../../SearchState/Presto/typings";
import {handlePrestoQueryCancel} from "../../Freeform/presto-search-requests";
import {handlePrestoGuidedQueryCancel} from "../../Guided/presto-guided-search-requests";
import styles from "./index.module.css";


/**
 * Renders a button to cancel the SQL query.
 *
 * @return
 */
const CancelButton = () => {
    const searchJobId = useSearchStore((state) => state.searchJobId);
    const aggregationJobId = useSearchStore((state) => state.aggregationJobId);
    const sqlInterface = usePrestoSearchState((state) => state.sqlInterface);
    const isPrestoGuided = sqlInterface === PRESTO_SQL_INTERFACE.GUIDED;

    const handleClick = useCallback(() => {
        if (null === searchJobId) {
            console.error("Cannot cancel query: searchJobId is not set.");

            return;
        }
        if (false === isPrestoGuided || null === aggregationJobId) {
            handlePrestoQueryCancel({searchJobId});
        } else {
            handlePrestoGuidedQueryCancel(searchJobId, aggregationJobId);
        }
    }, [aggregationJobId,
        searchJobId,
        isPrestoGuided]);

    return (
        <Tooltip title={"Cancel query"}>
            <Button
                className={styles["cancelButton"] || ""}
                color={"red"}
                htmlType={"button"}
                icon={<CloseOutlined/>}
                size={"middle"}
                variant={"solid"}
                onClick={handleClick}
            >
                Cancel
            </Button>
        </Tooltip>
    );
};

export default CancelButton;
