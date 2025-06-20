import {Typography} from "antd";

import {
    DashboardCard,
    DashboardCardProps,
} from "../DashboardCard/index";


const {Text} = Typography;

interface StatCardProps {
    title: string;
    stat: string;
    titleColor?: string;
    backgroundColor?: string;
    statSize?: string;
    statColor?: string;
}

/**
 * Renders a card with a statistic.
 *
 * @param props
 * @param props.title
 * @param props.stat
 * @param props.titleColor
 * @param props.backgroundColor
 * @param props.statSize
 * @param props.statColor
 * @return
 */
const StatCard = ({
    title,
    stat,
    titleColor,
    backgroundColor,
    statSize,
    statColor,
}: StatCardProps) => {
    const props: DashboardCardProps = {
        title,
        ...(titleColor ?
            {titleColor} :
            {}),
        ...(backgroundColor ?
            {backgroundColor} :
            {}),
    };

    return (
        <DashboardCard {...props}>
            <Text style={{color: statColor, fontSize: statSize}}>
                {stat}
            </Text>
        </DashboardCard>
    );
};
export default StatCard;
