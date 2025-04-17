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
 * @param root0
 * @param root0.title
 * @param root0.stat
 * @param root0.textColor
 * @param root0.backgroundColor
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
