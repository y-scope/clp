import {
    theme,
    Typography,
} from "antd";

import styles from "./index.module.css";

const {Text} = Typography;

/**
 * Renders a label for an input field.
 *
 * @param children - The label text to display.
 * @return
 */
const InputLabel = ({ children }: { children: React.ReactNode }) => {
    const {token} = theme.useToken();

    return (
        <Text
            className={styles["label"] || ""}
            style={{
                backgroundColor: token.colorFillTertiary,
                borderBottomLeftRadius: `${token.borderRadius}px`,
                borderColor: token.colorBorder,
                borderTopLeftRadius: `${token.borderRadius}px`,
                fontSize: token.fontSizeLG,
            }}
        >
            {children}
        </Text>
    );
};

export default InputLabel;
