import {
    theme,
    Typography,
} from "antd";
import styles from "./Label.module.css";

const { Text } = Typography;

/**
 * Renders a generic label for input fields, styled similarly to DatasetLabel.
 *
 * @param {React.ReactNode} children - The label text.
 * @return
 */
const Label = ({ children }: { children: React.ReactNode }) => {
    const { token } = theme.useToken();
    return (
        <Text
            className={styles["label"] || ""}
            style={{
                backgroundColor: token.colorFillTertiary,
                borderBottomLeftRadius: `${token.borderRadius}px`,
                borderColor: token.colorBorder,
                borderTopLeftRadius: `${token.borderRadius}px`,
                fontSize: token.fontSizeLG,
                padding: "0 16px",
                minWidth: 80,
                height: "100%",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
            }}
        >
            {children}
        </Text>
    );
};

export default Label;
