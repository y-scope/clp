import {
    theme,
    Typography,
} from "antd";

import styles from "./index.module.css";


const {Text} = Typography;

/**
 * Renders a label for the dataset selector.
 *
 * @return
 */
const DatasetLabel = () => {
    const {token} = theme.useToken();

    return (
        <Text
            className={styles["datasetLabel"] || ""}
            style={{
                backgroundColor: token.colorFillTertiary,
                borderBottomLeftRadius: `${token.borderRadius}px`,
                borderColor: token.colorBorder,
                borderTopLeftRadius: `${token.borderRadius}px`,
                fontSize: token.fontSizeLG,
            }}
        >
            Dataset
        </Text>
    );
};

export default DatasetLabel;
