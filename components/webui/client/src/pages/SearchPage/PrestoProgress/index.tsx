import {useEffect} from "react";

import {Progress} from "antd";

import {usePseudoProgress} from "../../../components/usePseudoProgress";
import useSearchStore from "../SearchState";
import {SEARCH_UI_STATE} from "../SearchState/typings";


/**
 * Renders a pseudo progress bar that listens for changes in `searchUiState`.
 *
 * @return
 */
const PrestoProgress = () => {
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
            percent={progress ?? 0}
            showInfo={false}
            size={"small"}
            status={"active"}
            strokeLinecap={"butt"}
            style={{lineHeight: 0,
                opacity: null === progress ?
                    0 :
                    100}}/>
    );
};

export {PrestoProgress};
