import {
    theme,
    Typography,
} from "antd";


const {Text} = Typography;

interface StatProps {
    text: string;
    fontSize?: string;
    color?: string;
}

/**
 * A text component for displaying statistical values.
 *
 * @param props
 * @param props.text
 * @param props.color
 * @param props.fontSize
 * @return
 */
const Stat = ({text, color, fontSize}: StatProps) => {
    const {token} = theme.useToken();
    return (
        <Text style={{color: color ?? token.colorTextSecondary, fontSize: fontSize ?? "1.4rem"}}>
            {text}
        </Text>
    );
};


export default Stat;
