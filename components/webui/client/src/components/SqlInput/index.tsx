// Reference: https://github.com/vikyd/vue-monaco-singleline
import {
    useCallback,
    useEffect,
    useRef,
} from "react";

import * as monaco from "monaco-editor/esm/vs/editor/editor.api.js";

import {ValidationError} from "../../sql-parser";
import SqlEditor, {
    SqlEditorProps,
    SqlEditorType,
} from "../SqlEditor";


type SqlInputProps = SqlEditorProps & {
    /**
     * Validation function to check SQL syntax and return errors.
     */
    validateFn?: (sqlString: string) => ValidationError[];
};

/**
 * Single-line SQL input.
 *
 * @param props
 * @return
 */
const SqlInput = (props: SqlInputProps) => {
    const {validateFn, ...editorProps} = props;
    const editorRef = useRef<SqlEditorType | null>(null);

    const handleEditorReady = useCallback((editor: SqlEditorType) => {
        editorRef.current = editor;

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

    // Validate SQL and update markers whenever value changes
    useEffect(() => {
        const editor = editorRef.current;
        if (!editor) {
            return;
        }

        const model = editor.getModel();
        if (!model) {
            return;
        }

        const value = "string" === typeof editorProps.value ?
            editorProps.value :
            "";

        // Clear markers if no validation function or empty/whitespace-only input
        if (!validateFn || !value.trim()) {
            monaco.editor.setModelMarkers(model, "sql-parser", []);

            return;
        }

        const errors = validateFn(value);
        const markers: monaco.editor.IMarkerData[] = errors.map((error) => ({
            endColumn: error.endColumn,
            endLineNumber: error.line,
            message: error.message,
            severity: monaco.MarkerSeverity.Error,
            startColumn: error.startColumn,
            startLineNumber: error.line,
        }));

        monaco.editor.setModelMarkers(model, "sql-parser", markers);
    }, [editorProps.value,
        validateFn]);

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
            {...editorProps}/>
    );
};

export default SqlInput;
export type {SqlInputProps};
