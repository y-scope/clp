import {Typography} from "antd";

import styles from "./index.module.css";
import TimestampKeySelect from "./TimestampKeySelect";


const {Text} = Typography;

/**
 * Renders the footer for the time range picker when in guided mode. Includes the timestamp key
 * selector.
 *
 * @return
 */
const TimeRangeFooter = () => {
    return (
        <div className={styles["footerContainer"]}>
            <Text>Timestamp key:</Text>
            <TimestampKeySelect
                className={styles["footerSelect"] || ""}
                size={"small"}/>
        </div>
    );
};

export default TimeRangeFooter;
