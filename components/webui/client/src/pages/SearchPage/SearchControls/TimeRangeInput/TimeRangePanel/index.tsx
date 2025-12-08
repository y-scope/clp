import React from "react";

import {theme} from "antd";

import useSearchStore from "../../../SearchState/index";
import {
    TIME_RANGE_OPTION,
    TIME_RANGE_OPTION_DAYJS_MAP,
    TIME_RANGE_OPTION_NAMES,
} from "../utils";
import styles from "./index.module.css";


interface TimeRangePanelProps {
    panelNode: React.ReactNode;
    onClose: () => void;
}

/**
 * Renders a time range selection panel with preset options and a custom date picker.
 *
 * @param props
 * @param props.panelNode The date picker panel node
 * @param props.onClose Callback to close the panel
 * @return
 */
const TimeRangePanel = ({panelNode, onClose}: TimeRangePanelProps) => {
    const {token} = theme.useToken();
    const updateTimeRange = useSearchStore((state) => state.updateTimeRange);
    const updateTimeRangeOption = useSearchStore((state) => state.updateTimeRangeOption);

    const handlePresetClick = (option: TIME_RANGE_OPTION) => {
        updateTimeRangeOption(option);
        TIME_RANGE_OPTION_DAYJS_MAP[option]()
            .then((dates) => {
                updateTimeRange([dates[0].utc(true),
                    dates[1].utc(true)]);
                onClose();
            })
            .catch((e: unknown) => {
                console.error("Failed to set time range:", e);
            });
    };

    return (
        <div className={styles["panelContainer"]}>
            <ul
                className={styles["presetList"]}
                style={{
                    padding: token.paddingXS,
                    borderInlineEnd: `${token.lineWidth}px ${token.lineType} ${token.colorSplit}`,
                }}
            >
                {TIME_RANGE_OPTION_NAMES
                    .filter((option) => option !== TIME_RANGE_OPTION.CUSTOM)
                    .map((option) => (
                        <li
                            className={styles["presetItem"]}
                            key={option}
                            style={{
                                borderRadius: token.borderRadiusSM,
                                paddingInline: token.paddingXS,
                                paddingBlock: token.paddingXXS,
                            }}
                            onClick={() => {
                                handlePresetClick(option);
                            }}
                            onMouseEnter={(e) => {
                                e.currentTarget.style.backgroundColor = token.controlItemBgHover;
                            }}
                            onMouseLeave={(e) => {
                                e.currentTarget.style.backgroundColor = "transparent";
                            }}
                        >
                            {option}
                        </li>
                    ))}
            </ul>
            {panelNode}
        </div>
    );
};

export default TimeRangePanel;
