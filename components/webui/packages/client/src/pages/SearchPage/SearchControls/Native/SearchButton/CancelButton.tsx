import {useCallback} from "react";

import {CloseOutlined} from "@ant-design/icons";
import {Button} from "antd";

import useSearchStore from "../../../SearchState/index";
import {handleQueryCancel} from "../search-requests";


/**
 * Renders a button to cancel the search query.
 *
 * @return
 */
const CancelButton = () => {
    const searchJobId = useSearchStore((state) => state.searchJobId);

    /**
     * Cancels search query.
     */
    const handleCancelButtonClick = useCallback(() => {
        // Note it should be impossible for searchJobId to be null,
        // since the button is only rendered when there is an active query.
        if (null === searchJobId) {
            console.error("Cannot cancel query: searchJobId is not set.");

            return;
        }
        handleQueryCancel(Number(searchJobId));
    }, [searchJobId]);

    return (
        <Button
            danger={true}
            htmlType={"button"}
            icon={<CloseOutlined/>}
            size={"middle"}
            type={"primary"}
            onClick={handleCancelButtonClick}
        >
            Cancel
        </Button>
    );
};

export default CancelButton;
