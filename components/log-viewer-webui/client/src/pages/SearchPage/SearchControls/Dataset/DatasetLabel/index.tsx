import {theme, Typography} from "antd";
import styles from "./index.module.css";

const {Text} = Typography;

const DatasetLabel = () => {
    const {token} = theme.useToken();

    return (
        <Text
            className={styles['datasetLabel'] || ""}
            style={{
                backgroundColor: token.colorFillAlter,
                borderColor: token.colorBorder,
                borderTopLeftRadius: `${token.borderRadius}px`,
                borderBottomLeftRadius: `${token.borderRadius}px`,
                fontSize: token.fontSizeLG
            }}
        >
            Dataset
        </Text>
    );
};

export default DatasetLabel;
