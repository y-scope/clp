import {
    Card,
    Typography,
} from "antd";

import styles from "./index.module.css";


const {Text} = Typography;

// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_MESSAGES = 1235844;

/**
 * Renders the messages statistic.
 *
 * @return
 */
const Messages = () => {
    return (
        <Card className={styles["card"] || ""}>
            <div className={styles["cardContent"]}>
                <Text className={styles["title"] || ""}>
                    Messages
                </Text>
                <Text className={styles["statistic"] || ""}>
                    {DUMMY_MESSAGES}
                </Text>
            </div>
        </Card>
    );
};

export default Messages;
