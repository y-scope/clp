import {
    Card,
    Typography,
} from "antd";

import styles from "./index.module.css";


const {Text} = Typography;

// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_COMPRESSED_SIZE = 1004023;
const DUMMY_UNCOMPRESSED_SIZE = 110300010;

/**
 * Presents space savings or compression ratio from the given statistics.
 *
 * @return
 */
const SpaceSavings = () => {
    const compressedSize = DUMMY_COMPRESSED_SIZE as number;
    const uncompressedSize = DUMMY_UNCOMPRESSED_SIZE as number;

    const spaceSavingsPercent = (0 === uncompressedSize) ?
        100 * (1 - (compressedSize / uncompressedSize)) :
        0;

    return (
        <Card
            className={styles["card"] || ""}
            hoverable={true}
        >
            <div className={styles["cardContent"]}>
                <Text className={styles["title"] || ""}>
                    Space Savings
                </Text>
                <Text className={styles["statistic"] || ""}>
                    {spaceSavingsPercent.toFixed(2)}
                    %
                </Text>
            </div>
        </Card>
    );
};


export default SpaceSavings;
