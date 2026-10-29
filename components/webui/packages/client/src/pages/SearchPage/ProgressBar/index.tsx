import {useEffect} from "react";

import {Progress} from "antd";

import useSearchStore from "../SearchState";
import {SEARCH_UI_STATE} from "../SearchState/typings";
import {usePseudoProgress} from "../SearchState/usePseudoProgress";
import styles from "./index.module.css";


/**
 * Renders a pseudo progress bar that listens for changes in `searchUiState`.
 *
 * @return
 */
const ProgressBar = () => {
    const {searchUiState} = useSearchStore.getState();
    const {
        progress,
        start,
        stop,
    } = usePseudoProgress();

    useEffect(() => {
        if (searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING) {
            start();
        } else if (
            searchUiState === SEARCH_UI_STATE.DONE ||
            searchUiState === SEARCH_UI_STATE.FAILED
        ) {
            stop();
        }
    }, [searchUiState,
        start,
        stop]);

    return (
        <Progress
            className={styles["prestoProgress"] || ""}
            percent={progress ?? 0}
            showInfo={false}
            size={"small"}
            status={"active"}
            strokeLinecap={"butt"}
            style={{display: null === progress ?
                "none" :
                "block"}}/>
    );
};

export {ProgressBar};
