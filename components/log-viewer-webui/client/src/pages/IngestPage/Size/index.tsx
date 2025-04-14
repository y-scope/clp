import { Card, Typography, Segmented } from 'antd';
import styles from './index.module.css';
import { useState } from 'react';
import { PercentageOutlined, CloseOutlined } from '@ant-design/icons';
import {SIZE_OPTION, DEFAULT_SIZE_OPTION} from "./typings.js"

const { Text } = Typography;

// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_COMPRESSED_SIZE = 1004023 ;
const DUMMY_UNCOMPRESSED_SIZE = 110300010;

const Size = () => {
    const [segmentedOption, setSegmentedOption] = useState<string>(DEFAULT_SIZE_OPTION);
    const compressedSize = DUMMY_COMPRESSED_SIZE;
    const uncompressedSize = DUMMY_UNCOMPRESSED_SIZE;

    const spaceSavingsPercent = 0 < compressedSize ?
        100 * (1 - (compressedSize / uncompressedSize)) : 0;

    const compressionRatio = 0 < compressedSize ?
        uncompressedSize / compressedSize : 0;

    const handleChange = (option: SIZE_OPTION) => {
        setSegmentedOption(option);
    };

    return (
        <Card
            hoverable
            className={styles['card'] || ""}>
                <div className={styles['cardContent']}>
                    <div className={styles['header']}>
                        <Text className={styles['title'] || ""}>
                            {segmentedOption}
                        </Text>
                        <Segmented
                            options={[
                                { value: SIZE_OPTION.SPACE_SAVINGS, icon: <PercentageOutlined /> },
                                { value: SIZE_OPTION.COMPRESSION_RATIO, icon: <CloseOutlined />},
                            ]}
                            defaultValue={DEFAULT_SIZE_OPTION}
                            onChange={handleChange}
                            className={styles['segmented'] || ""}
                        />
                    </div>
                        <Text className={styles['statistic'] || ""}>
                            {segmentedOption === SIZE_OPTION.SPACE_SAVINGS
                                ? `${spaceSavingsPercent.toFixed(2)}%`
                                : `${compressionRatio.toFixed(2)}x`}
                        </Text>
                </div>
        </Card>
    );
};

export default Size;
