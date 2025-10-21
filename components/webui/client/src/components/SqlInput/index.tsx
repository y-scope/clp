// Reference: https://github.com/vikyd/vue-monaco-singleline
import {useCallback} from "react";

import SqlEditor, {
    SqlEditorProps,
    SqlEditorType,
} from "../SqlEditor";

import * as monaco from "monaco-editor/esm/vs/editor/editor.api.js";


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

        let formEl = editor.getDomNode()?.closest("form") as HTMLFormElement | null;


                // Helper that behaves like a real submit
        const submit = () => {
            console.log(formEl)
            if(formEl) {
                formEl.dispatchEvent(new Event("submit", { bubbles: true, cancelable: true }));
            }
        };

        // 2) Register a Monaco Action (shows in context menu & supports keybindings)
            editor.addAction({
              id: "run-query",
              label: "Run Query",
              // Use Cmd/Ctrl+Enter; avoids clobbering plain Enter/newlines
              keybindings: [monaco.KeyCode.Enter],
              run: () => submit(),
        });


    }, []);

    return (
        <SqlEditor
            options={{
                automaticLayout: true,
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
                    top: 6,
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
