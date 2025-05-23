import { CloseOutlined } from "@ant-design/icons";
import { Button } from "antd";
import { handleQueryCancel } from "../../SearchState/query";
import styles from "./index.module.css";

const CancelButton = () => (
    <Button
        //className={styles["gradientButtonCancel"] || ""}
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
