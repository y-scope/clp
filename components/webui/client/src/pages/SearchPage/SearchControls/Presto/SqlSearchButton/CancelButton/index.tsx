import {useCallback} from "react";

import {CloseOutlined} from "@ant-design/icons";
import {
    Button,
    Tooltip,
} from "antd";

import useSearchStore from "../../../../SearchState/index";
import {handlePrestoQueryCancel} from "../../presto-search-requests";
import styles from "./index.module.css";


/**
 * Renders a button to cancel the SQL query.
 *
 * @return
 */
const CancelButton = () => {
    const searchJobId = useSearchStore((state) => state.searchJobId);

    const handleClick = useCallback(() => {
        if (null === searchJobId) {
            console.error("Cannot cancel query: searchJobId is not set.");

            return;
        }
        handlePrestoQueryCancel({searchJobId});
    }, [searchJobId]);

    return (
        <Tooltip title={"Cancel query"}>
            <Button
                className={styles["cancelButton"] || ""}
                color={"red"}
                icon={<CloseOutlined/>}
                size={"large"}
                variant={"solid"}
                onClick={handleClick}
            >
                Cancel
            </Button>
        </Tooltip>
    );
};

export default CancelButton;
