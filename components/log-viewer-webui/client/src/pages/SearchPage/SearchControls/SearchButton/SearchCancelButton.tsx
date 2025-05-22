import { CloseOutlined } from "@ant-design/icons";
import { Button } from "antd";
import { handleQueryCancel } from "../../SearchState/query";
import styles from "./index.module.css";

const SearchCancelButton = () => (
    <Button
        //className={styles["gradientButtonCancel"] || ""}
        icon={<CloseOutlined />}
        color="danger" variant="filled"
        size="large"
        onClick={() => {
            handleQueryCancel().catch((error) => {
                console.error("Error during query cancel:", error);
            });
        }}
    >
        Cancel
    </Button>
);

export default SearchCancelButton;
