import {theme} from "antd";

import StatCard from "../../../components/StatCard";


interface DetailsCardProps {
    title: string;
    stat: string;
    isLoading: boolean;
}

/**
 * A stat card in the details grid.
 *
 * @param props
 * @param props.title
 * @param props.stat
 * @param props.isLoading
 * @return
 */
const DetailsCard = ({title, stat, isLoading}: DetailsCardProps) => {
    const {token} = theme.useToken();
    return (
        <StatCard
            isLoading={isLoading}
            stat={stat}
            statColor={token.colorTextSecondary}
            statSize={"1.4rem"}
            title={title}/>
    );
};

export default DetailsCard;
