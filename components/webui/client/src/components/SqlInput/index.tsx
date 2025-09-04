// Reference: https://github.com/vikyd/vue-monaco-singleline/tree/master
import {useCallback} from "react";
import SqlEditor, {SqlEditorType} from "../SqlEditor";

type SqlInputProps = {
    disabled: boolean;
    onChange?: (value: string | undefined) => void;
};

/**
 * Single-line SQL input.
 */
const SqlInput = (props: SqlInputProps) => {

    const handleEditorReady = useCallback((editor: SqlEditorType) => {
        // Prevent multi-line input by repositioning cursor and replacing newlines with empty string.
        editor.onDidChangeCursorPosition((e) => {
            if (e.position.lineNumber > 1) {
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
                renderLineHighlight: "none",
                folding: false,
                minimap: {enabled: false},
                wordWrap: "off",
                lineNumbers: "off",
                glyphMargin: false,
                overviewRulerBorder: false,
                hideCursorInOverviewRuler: true,
                scrollbar: {
                    horizontal: "hidden",
                    vertical: "hidden",
                    alwaysConsumeMouseWheel: false,
                },
                padding: {
                    top: 4,
                    bottom: 4,
                },
                scrollBeyondLastLine: false,
                roundedSelection: false,
                find: {
                    addExtraSpaceOnTop: false,
                    autoFindInSelection: "never",
                    seedSearchStringFromSelection: "never",
                },
                fixedOverflowWidgets: true,
                scrollBeyondLastColumn: 0,
                lineNumbersMinChars: 0,
                overviewRulerLanes: 0,
            }}
            onEditorReady={handleEditorReady}
            {...props}
        />
    );
};

export default SqlInput;
