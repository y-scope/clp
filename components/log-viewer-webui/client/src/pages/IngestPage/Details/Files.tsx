import {
    Card,
    Typography,
} from "antd";

import styles from "./index.module.css";


const {Text} = Typography;

// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_FILES = 124;

/**
 * Renders the files statistic.
 *
 * @return
 */
const Files = () => {
    return (
        <Card className={styles["card"] || ""}>
            <div className={styles["cardContent"]}>
                <Text className={styles["title"] || ""}>
                    Files
                </Text>
                <Text className={styles["statistic"] || ""}>
                    {DUMMY_FILES}
                </Text>
            </div>
        </Card>
    );
};

export default Files;
