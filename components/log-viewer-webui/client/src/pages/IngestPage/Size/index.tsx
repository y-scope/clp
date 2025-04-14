import {useState} from "react";

import {
    CloseOutlined,
    PercentageOutlined,
} from "@ant-design/icons";
import {
    Card,
    Segmented,
    Typography,
} from "antd";

import styles from "./index.module.css";
import {
    DEFAULT_SIZE_OPTION,
    SIZE_OPTION,
} from "./typings.js";


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
const Size = () => {
    const [segmentedOption, setSegmentedOption] = useState<SIZE_OPTION>(DEFAULT_SIZE_OPTION);
    const compressedSize = DUMMY_COMPRESSED_SIZE;
    const uncompressedSize = DUMMY_UNCOMPRESSED_SIZE;

    // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
    const spaceSavingsPercent = 0 < compressedSize ?
        100 * (1 - (compressedSize / uncompressedSize)) :
        0;

    // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
    const compressionRatio = 0 < compressedSize ?
        uncompressedSize / compressedSize :
        0;

    const handleChange = (option: SIZE_OPTION) => {
        setSegmentedOption(option);
    };

    return (
        <Card
            className={styles["card"] || ""}
            hoverable={true}
        >
            <div className={styles["cardContent"]}>
                <div className={styles["header"]}>
                    <Text className={styles["title"] || ""}>
                        {segmentedOption}
                    </Text>
                    <Segmented
                        className={styles["segmented"] || ""}
                        defaultValue={DEFAULT_SIZE_OPTION}
                        options={[
                            {value: SIZE_OPTION.SPACE_SAVINGS, icon: <PercentageOutlined/>},
                            {value: SIZE_OPTION.COMPRESSION_RATIO, icon: <CloseOutlined/>},
                        ]}
                        onChange={handleChange}/>
                </div>
                <Text className={styles["statistic"] || ""}>
                    {segmentedOption === SIZE_OPTION.SPACE_SAVINGS ?
                        `${spaceSavingsPercent.toFixed(2)}%` :
                        `${compressionRatio.toFixed(2)}x`}
                </Text>
            </div>
        </Card>
    );
};

export default Size;
