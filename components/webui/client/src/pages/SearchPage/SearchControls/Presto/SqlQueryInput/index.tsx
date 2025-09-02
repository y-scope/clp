import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import {Nullable} from "src/typings/common";

import SqlEditor, {SqlEditorRef} from "../../../../../components/SqlEditor";
import useSearchStore from "../../../SearchState/index";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import styles from "./index.module.css";


/**
 * Renders SQL query input.
 *
 * @return
 */
const SqlQueryInput = () => {
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const editorRef = useRef<Nullable<SqlEditorRef>>(null);
    const [isEditorReady, setIsEditorReady] = useState(false);

    const handleChange = useCallback((value: string | undefined) => {
        const {updateQueryString} = useSearchStore.getState();
        updateQueryString(value || "");
    }, []);

    const handleEditorReady = useCallback(() => {
        setIsEditorReady(true);
    }, []);

    useEffect(() => {
        if (
            isEditorReady &&
            (searchUiState === SEARCH_UI_STATE.DEFAULT ||
            searchUiState === SEARCH_UI_STATE.DONE ||
            searchUiState === SEARCH_UI_STATE.FAILED)
        ) {
            editorRef.current?.focus();
        }
    }, [
        searchUiState,
        isEditorReady,
    ]);

    return (
        <div className={styles["input"] || ""}>
            <SqlEditor
                height={"120px"}
                ref={editorRef}
                disabled={
                    searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
                    searchUiState === SEARCH_UI_STATE.QUERYING
                }
                onChange={handleChange}
                onEditorReady={handleEditorReady}/>
        </div>
    );
};

export default SqlQueryInput;
