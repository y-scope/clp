import { CloseOutlined } from "@ant-design/icons";
import { Button } from "antd";
import { handleQueryCancel } from "../../SearchState/query";

const CancelButton = () => (
    <Button
        icon={<CloseOutlined />}
        danger
        type = "primary"
        size="large"
        onClick={() => {
            handleQueryCancel()
        }}
    >
        Cancel
    </Button>
);

export default CancelButton;
