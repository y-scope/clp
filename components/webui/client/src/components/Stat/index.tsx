import {
    theme,
    Typography,
} from "antd";


const {Text} = Typography;

interface StatCardProps {
    children: React.ReactNode;
    fontSize?: string;
    color?: string;
}

/**
 * A text component for displaying statistical values in `DashboardCard`.
 *
 * @param props
 * @param props.children
 * @param props.color
 * @param props.fontSize
 * @return
 */
const Stat = ({children, color, fontSize}: StatCardProps) => {
    const {token} = theme.useToken();
    return (
        <Text style={{color: color ?? token.colorTextSecondary, fontSize: fontSize ?? "1.4rem"}}>
            {children}
        </Text>
    );
};


export default Stat;
