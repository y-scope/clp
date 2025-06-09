import {
    Input,
    InputProps,
    Progress,
    theme,
} from "antd";
import {Nullable} from "src/typings/common";

import styles from "./index.module.css";
import CaseSensitiveToggle, { CaseSensitiveToggleProps } from "./CaseSenstiveToggle";


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
                    disabled={inputProps.disabled}
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
