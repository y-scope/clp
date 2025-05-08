import {
    Input,
    InputProps,
    Progress,
} from "antd";

import {Nullable} from "../../typings/common";
import styles from "./index.module.css";

export interface QueryBoxProps extends InputProps {
    /** The progress of the progress bar from `0` to `100`. Hide the bar if null. */
    progress: Nullable<number>;
}

/**
 * Renders an Input with a progress bar.
 *
 * @param queryBoxProps
 * @return
 */
const QueryBox = (queryBoxProps: QueryBoxProps) => {
    const inputProps = queryBoxProps as InputProps;
    const {progress} = queryBoxProps;
    let progressStyle;
    let percent = 0;
    if (null === progress) {
        progressStyle = {display: "none"};
    } else {
        percent = Math.max(Math.min(progress, 100), 0);
        progressStyle = {display: "block"};
    }

    return (
        <div className={styles["queryBox"]}>
            <Input {...inputProps}/>
            <div className={styles["progressWrapper"]}>
                <Progress
                    percent={percent}
                    showInfo={false}
                    size={"small"}
                    strokeLinecap={"butt"}
                    style={progressStyle}/>
            </div>
        </div>
    );
};
export default QueryBox;
