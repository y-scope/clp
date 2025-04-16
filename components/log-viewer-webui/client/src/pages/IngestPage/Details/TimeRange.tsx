import {
    Card,
    Typography,
} from "antd";
import dayjs from "dayjs";

import styles from "./index.module.css";


const {Text} = Typography;

// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_START_DATE = "2021-12-14";
const DUMMY_END_DATE = "2025-04-16";

const DATE_FORMAT = "MMMM D, YYYY";

/**
 * Renders the time range statistic.
 *
 * @return
 */
const TimeRange = () => {
    return (
        <Card className={styles["card"] || ""}>
            <div className={styles["cardContent"]}>
                <Text className={styles["title"] || ""}>
                    Time Range
                </Text>
                <Text className={styles["statistic"] || ""}>
                    {`${dayjs(DUMMY_START_DATE).format(DATE_FORMAT)} - ${
                        dayjs(DUMMY_END_DATE).format(DATE_FORMAT)
                    }`}
                </Text>
            </div>
        </Card>
    );
};

export default TimeRange;
