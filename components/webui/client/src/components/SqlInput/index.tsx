// Reference: https://github.com/vikyd/vue-monaco-singleline
import {useCallback} from "react";

import SqlEditor, {
    SqlEditorProps,
    SqlEditorType,
} from "../SqlEditor";


/**
 * Single-line SQL input.
 *
 * @param props
 * @return
 */
const SqlInput = (props: SqlEditorProps) => {
    const handleEditorReady = useCallback((editor: SqlEditorType) => {
        // Prevent multi-line input by repositioning cursor and replacing newlines with empty
        // string.
        editor.onDidChangeCursorPosition((e) => {
            if (1 < e.position.lineNumber) {
                editor.setValue(editor.getValue().replace(/\r?\n/g, ""));
                editor.setPosition({
                    lineNumber: 1,
                    column: Infinity,
                });
            }
        });
    }, []);

    return (
        <SqlEditor
            height={30}
            options={{
                find: {
                    addExtraSpaceOnTop: false,
                    autoFindInSelection: "never",
                    seedSearchStringFromSelection: "never",
                },
                fixedOverflowWidgets: true,
                folding: false,
                glyphMargin: false,
                hideCursorInOverviewRuler: true,
                lineNumbers: "off",
                lineNumbersMinChars: 0,
                minimap: {enabled: false},
                overviewRulerBorder: false,
                overviewRulerLanes: 0,
                padding: {
                    top: 4,
                    bottom: 4,
                },
                renderLineHighlight: "none",
                roundedSelection: false,
                scrollBeyondLastColumn: 0,
                scrollBeyondLastLine: false,
                scrollbar: {
                    horizontal: "hidden",
                    vertical: "hidden",
                    alwaysConsumeMouseWheel: false,
                },
                wordWrap: "off",
            }}
            onEditorReady={handleEditorReady}
            {...props}/>
    );
};

export default SqlInput;
