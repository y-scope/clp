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
 * @param queryBoxProps
 * @param queryBoxProps.progress
 * @param queryBoxProps.rest
 * @return
 */
const QueryBox = ({progress, ...rest}: QueryBoxProps) => {
    return (
        <div className={styles["queryBox"]}>
            <div className={styles["queryBoxPosition"]}>
                <Input {...rest}/>
                <div className={styles["queryBoxWrapper"]}>
                    <Progress
                        className={styles["queryBoxProgress"] || ""}
                        percent={progress ?? 0}
                        showInfo={false}
                        size={"small"}
                        strokeLinecap={"butt"}
                        style={{display: null === progress ?
                            "none" :
                            "block"}}/>
                </div>
            </div>
        </div>
    );
};
export default QueryBox;
