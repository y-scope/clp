import {
    theme,
    Typography,
} from "antd";

import styles from "./index.module.css";

const {Text} = Typography;

/**
 * Renders a label for an input field.
 *
 * @param children The label text to display.
 * @param [fontSize] Font size for the label.
 * @return
 */
const InputLabel = ({ children, fontSize }: { children: React.ReactNode, fontSize?: number | string }) => {
    const {token} = theme.useToken();
    const resolvedFontSize = fontSize || token.fontSize;

    return (
        <Text
            className={styles["label"] || ""}
            style={{
                backgroundColor: token.colorFillTertiary,
                borderBottomLeftRadius: `${token.borderRadius}px`,
                borderColor: token.colorBorder,
                borderTopLeftRadius: `${token.borderRadius}px`,
                fontSize: resolvedFontSize,
            }}
        >
            {children}
        </Text>
    );
};

export default InputLabel;
