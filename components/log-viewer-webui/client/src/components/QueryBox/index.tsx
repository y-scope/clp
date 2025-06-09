import {
    Progress,
    theme,
} from "antd";
import {Nullable} from "src/typings/common";

import styles from "./index.module.css";
import InputWithCaseSensitive, {InputWithCaseSensitiveProps} from "./InputWithCaseSenstive";


interface QueryBoxProps extends InputWithCaseSensitiveProps {
    /**
     * The progress of the progress bar from `0` to `100`. Hides the bar if `null`.
     */
    progress: Nullable<number>;
}

/**
 * Renders an Input with a progress bar.
 *
 * @param props
 * @param props.inputProps
 * @param props.progress The progress value for the progress bar
 * @return
 */
const QueryBox = ({
    progress,
    ...inputProps
}: QueryBoxProps) => {
    const {token} = theme.useToken();
    return (
        <div className={styles["queryBox"]}>
            <InputWithCaseSensitive {...inputProps}/>
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
