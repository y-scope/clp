import {
    Card,
    Typography,
} from "antd";

import styles from "./index.module.css";


const {Text} = Typography;

interface StatCardProps {
    title: string;
    stat: string;
    textColor?: string;
    backgroundColor?: string;
}

/**
 * Renders a card that dislays a statistic.
 *
 * @param props
 * @param props.title
 * @param props.stat
 * @param props.textColor
 * @param props.backgroundColor
 * @return
 */
const StatCard = ({title, stat, textColor, backgroundColor}: StatCardProps) => {
    return (
        <Card
            className={styles["card"] || ""}
            hoverable={true}
            style={{backgroundColor}}
        >
            <div className={styles["cardContent"]}>
                <Text
                    className={styles["title"] || ""}
                    style={{color: textColor}}
                >
                    {title}
                </Text>
                <Text
                    className={styles["statistic"] || ""}
                    style={{color: textColor}}
                >
                    {stat}
                </Text>
            </div>
        </Card>
    );
};

export default StatCard;
