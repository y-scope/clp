import {useCallback} from "react";

import {
    Button,
    Input,
    InputProps,
    Progress,
    theme,
    Tooltip,
    Typography,
} from "antd";
import {Nullable} from "src/typings/common";

import styles from "./index.module.css";


interface CaseSensitiveToggleProps {
    isCaseSensitive: boolean;
    onCaseSensitiveChange: (newValue: boolean) => void;
}

/**
 * A toggle button component that switches between case-sensitive and case-insensitive modes.
 *
 * This component displays a button labeled "Aa". When clicked, it toggles the `isCaseSensitive`
 * state and invokes the `onCaseSensitiveChange` callback with the new value.
 *
 * @param props
 * @param props.isCaseSensitive
 * @param props.onCaseSensitiveChange
 * @return
 */
const CaseSensitiveToggle = ({
    isCaseSensitive,
    onCaseSensitiveChange,
}: CaseSensitiveToggleProps) => {
    const handleButtonClick = useCallback(() => {
        onCaseSensitiveChange(!isCaseSensitive);
    }, [
        isCaseSensitive,
        onCaseSensitiveChange,
    ]);

    return (
        <Tooltip title={"Match case"}>
            <Button
                color={"default"}
                icon={<Typography.Text disabled={!isCaseSensitive}>Aa</Typography.Text>}
                size={"small"}
                variant={isCaseSensitive ?
                    "filled" :
                    "text"}
                onClick={handleButtonClick}/>
        </Tooltip>
    );
};


interface QueryBoxProps extends InputProps, CaseSensitiveToggleProps {
    /**
     * The progress of the progress bar from `0` to `100`. Hides the bar if `null`.
     */
    progress: Nullable<number>;
}

/**
 * Renders an Input with a progress bar.
 *
 * @param props
 * @param props.progress
 * @param props.inputProps
 * @param props.onCaseSensitiveChange
 * @param props.isCaseSensitive
 * @return
 */
const QueryBox = ({
    progress,
    isCaseSensitive,
    onCaseSensitiveChange,
    ...inputProps
}: QueryBoxProps) => {
    const {token} = theme.useToken();
    return (
        <div className={styles["queryBox"]}>
            <Input
                {...inputProps}
                suffix={<CaseSensitiveToggle
                    isCaseSensitive={isCaseSensitive}
                    onCaseSensitiveChange={onCaseSensitiveChange}/>}/>
            <div
                className={styles["progressBarMask"]}
                style={{borderRadius: token.borderRadiusLG}}
            >
                {null !== progress && (
                    <Progress
                        className={styles["progressBar"] || ""}
                        percent={progress}
                        showInfo={false}
                        size={"small"}
                        status={"active"}
                        strokeLinecap={"butt"}/>
                )}
            </div>
        </div>
    );
};

export type {QueryBoxProps};
export default QueryBox;
