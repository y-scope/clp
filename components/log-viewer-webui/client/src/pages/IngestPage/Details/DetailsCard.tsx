import {theme} from "antd";

import StatCard from "../../../components/StatCard";


interface DetailsCardProps {
    title: string;
    stat: string;
}

/**
 * A stat card in the details grid.
 *
 * @param props
 * @param props.title
 * @param props.stat
 * @return
 */
const DetailsCard = ({title, stat}: DetailsCardProps) => {
    const {token} = theme.useToken();
    return (
        <StatCard
            stat={stat}
            statColor={token.colorTextSecondary}
            statSize={"1.4rem"}
            title={title}/>
    );
};

export default DetailsCard;
