import {useCallback} from "react";

import {CloseOutlined} from "@ant-design/icons";
import {Button} from "antd";

import useSearchStore from "../../SearchState/index";
import {handleQueryCancel} from "../search-requests";


/**
 * Renders a button to cancel the search query.
 *
 * @return
 */
const CancelButton = () => {
    const {searchJobId, aggregationJobId} = useSearchStore();

    /**
     * Cancels search query.
     */
    const handleCancelButtonClick = useCallback(() => {
        // Note it should be impossible to for searchJobId or aggregationJobId to be null,
        // since the button is only rendered when there is an active query.
        if (null === searchJobId) {
            console.error("Cannot cancel query: searchJobId is not set.");

            return;
        }
        if (null === aggregationJobId) {
            console.error("Cannot cancel query: aggregationJobId is not set.");

            return;
        }
        handleQueryCancel(
            {searchJobId, aggregationJobId}
        );
    }, [searchJobId,
        aggregationJobId]);

    return (
        <Button
            danger={true}
            htmlType={"button"}
            icon={<CloseOutlined/>}
            size={"large"}
            type={"primary"}
            onClick={handleCancelButtonClick}
        >
            Cancel
        </Button>
    );
};

export default CancelButton;
