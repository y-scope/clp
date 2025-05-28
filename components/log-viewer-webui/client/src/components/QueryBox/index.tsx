import {
    Input,
    InputProps,
    Progress,
    theme,
} from "antd";

import styles from "./index.module.css";

export interface QueryBoxProps extends InputProps {
    /**
     * The progress of the progress bar from `0` to `100`. Hides the bar if `0`.
     */
    progress: number;
}

/**
 * Renders an Input with a progress bar.
 *
 * @param props
 * @param props.progress
 * @param props.inputProps
 * @return
 */
const QueryBox = ({progress, ...inputProps}: QueryBoxProps) => {
    const {token} = theme.useToken();

    return (
        <div className={styles["queryBox"]}>
            <Input {...inputProps}/>
            <div
                className={styles["progressBarMask"]}
                style={{borderRadius: token.borderRadius}}
            >
                {0 !== progress && (
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
export default QueryBox;
