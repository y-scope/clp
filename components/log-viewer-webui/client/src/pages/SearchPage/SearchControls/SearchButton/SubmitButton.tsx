import { SearchOutlined } from "@ant-design/icons";
import { Button, Tooltip } from "antd";
import { handleQuerySubmit } from "../../SearchState/query";
import useSearchStore, { SEARCH_STATE_DEFAULT } from "../../SearchState/index";
import styles from "./index.module.css";

const SubmitButton = () => {
    const queryString = useSearchStore((state) => state.queryString);
    const isQueryStringEmpty = queryString === SEARCH_STATE_DEFAULT.queryString;

    return (
        <Tooltip title={isQueryStringEmpty ? "Enter query to search" : ""}>
            <Button
                className={styles["gradientButton"] || ""}
                disabled={isQueryStringEmpty}
                icon={<SearchOutlined />}
                size="large"
                type="primary"
                onClick={() => {
                    handleQuerySubmit();
                }}
            >
                Search
            </Button>
        </Tooltip>
    );
};

export default SubmitButton;
