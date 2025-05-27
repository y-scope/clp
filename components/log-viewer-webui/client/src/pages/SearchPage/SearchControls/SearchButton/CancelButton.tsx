import { CloseOutlined } from "@ant-design/icons";
import { Button } from "antd";
import { handleQueryCancel } from "../../SearchState/query";

/**
 * Renders a button to cancel the search query.
 *
 * @return
 */
const CancelButton = () => (
    <Button
        icon={<CloseOutlined />}
        danger
        type="primary"
        size="large"
        onClick={() => {
            handleQueryCancel()
        }}
    >
        Cancel
    </Button>
);

export default CancelButton;
