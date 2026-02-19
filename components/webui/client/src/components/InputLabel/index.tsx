import {
    theme,
    Typography,
} from "antd";

import styles from "./index.module.css";


const {Text} = Typography;

/**
 * Renders a label for an input field.
 *
 * @param props
 * @param props.children The label text to display.
 * @param props.width Optional width for the label.
 * @return
 */
const InputLabel = ({children, width}: {children: React.ReactNode; width?: string | number}) => {
    const {token} = theme.useToken();

    return (
        <Text
            className={styles["label"] || ""}
            style={{
                backgroundColor: token.colorBorder,
                borderBottomLeftRadius: `${token.borderRadius}px`,
                borderTopLeftRadius: `${token.borderRadius}px`,
                fontSize: token.fontSize,
                minWidth: width,
                width: width,
            }}
        >
            {children}
        </Text>
    );
};

export default InputLabel;
