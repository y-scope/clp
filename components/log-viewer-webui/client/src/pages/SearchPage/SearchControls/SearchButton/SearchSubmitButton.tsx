import { SearchOutlined } from "@ant-design/icons";
import { Button } from "antd";
import { handleQuerySubmit } from "../../SearchState/query";
import styles from "./index.module.css";

type Props = {
    queryString: string;
    isQueryStringEmpty: boolean;
    className?: string;
};

const SearchSubmitButton = ({ queryString, isQueryStringEmpty}: Props) => (
    <Button
        className={styles["gradientButton"] || ""}
        disabled={isQueryStringEmpty}
        icon={<SearchOutlined />}
        size="large"
        type="primary"
        onClick={() => {
            handleQuerySubmit({
                queryString,
                timestampBegin: 0,
                timestampEnd: Date.now(),
                ignoreCase: false,
                timeRangeBucketSizeMillis: 150,
            }).catch((error) => {
                console.error("Error during query submission:", error);
            });
        }}
    >
        Search
    </Button>
);

export default SearchSubmitButton;
