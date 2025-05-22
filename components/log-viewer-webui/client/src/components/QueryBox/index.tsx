import {
    Input,
    InputProps,
    Progress,
} from "antd";

import {Nullable} from "../../typings/common";
import styles from "./index.module.css";

export interface QueryBoxProps extends InputProps {
    /** The progress of the progress bar from `0` to `100`. Hides the bar if null. */
    progress: Nullable<number>;
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
    return (
        <div className={styles["queryBox"]}>
                <Input {...inputProps}/>
                <div className={styles["progressBarMask"]}>
                    <Progress
                        className={styles["progressBar"] || ""}
                        percent={progress ?? 0}
                        showInfo={false}
                        size={"small"}
                        strokeLinecap={"butt"}
                        style={{display: null === progress ?
                            "none" :
                            "block"}}/>
                </div>
        </div>
    );
};
export default QueryBox;
