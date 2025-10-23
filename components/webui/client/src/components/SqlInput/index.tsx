// Reference: https://github.com/vikyd/vue-monaco-singleline
import {useCallback} from "react";

import * as monaco from "monaco-editor/esm/vs/editor/editor.api.js";

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

        // Add action to simulate form submission on Enter key
        editor.addAction({
            id: "run-query",
            label: "Run Query",
            keybindings: [monaco.KeyCode.Enter],
            run: () => {
                const formEl = editor.getDomNode()?.closest("form");
                const submitBtn = formEl?.querySelector<HTMLButtonElement>('button[type="submit"]');
                submitBtn?.click();
            },
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
