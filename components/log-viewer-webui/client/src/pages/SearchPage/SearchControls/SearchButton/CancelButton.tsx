import {CloseOutlined} from "@ant-design/icons";
import {Button} from "antd";

import useSearchStore from "../../SearchState/index";
import {handleQueryCancel} from "../requests";


/**
 * Renders a button to cancel the search query.
 *
 * @return
 */
const CancelButton = () => {
    const {searchJobId, aggregationJobId} = useSearchStore();

    return (
        <Button
            danger={true}
            icon={<CloseOutlined/>}
            size={"large"}
            type={"primary"}
            onClick={() => {
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
            }}
        >
            Cancel
        </Button>
    );
};

export default CancelButton;
