import { CloseOutlined } from "@ant-design/icons";
import { Button } from "antd";
import { handleQueryCancel } from "../requests";
import useSearchStore from "../../SearchState/index";

/**
 * Renders a button to cancel the search query.
 *
 * @return
 */
const CancelButton = () => {
    const {searchJobId, aggregationJobId} = useSearchStore();

    return (
        <Button
            icon={<CloseOutlined />}
            danger
            type="primary"
            size="large"
            onClick={() => {
                if (null === searchJobId || null === aggregationJobId) {
                    console.error("Cannot cancel query: searchJobId or aggregationJobId is not set.");
                    return;
                }
                handleQueryCancel(
                    { searchJobId, aggregationJobId }
                )
            }}
        >
            Cancel
        </Button>
    )
};

export default CancelButton;
