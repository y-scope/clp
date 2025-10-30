// Reference: https://github.com/vikyd/vue-monaco-singleline
import {
    useCallback,
    useEffect,
    useRef,
} from "react";

import * as monaco from "monaco-editor/esm/vs/editor/editor.api.js";

import SqlEditor, {
    SqlEditorProps,
    SqlEditorType,
} from "../SqlEditor";
import {ValidationError} from "../../sql-parser";


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

        const value = typeof editorProps.value === "string" ? editorProps.value : "";

        // Clear markers if no validation function or empty/whitespace-only input
        if (!validateFn || !value.trim()) {
            monaco.editor.setModelMarkers(model, "sql-parser", []);

            return;
        }

        const errors = validateFn(value);
        const markers: monaco.editor.IMarkerData[] = errors.map((error) => {
            const line = model.getLineContent(error.line);
            const startColumn = error.column + 1;
            // Try to find the end of the token by looking for whitespace or end of line
            let endColumn = startColumn + 1;
            for (let i = error.column; i < line.length; i++) {
                const char = line[i];
                if ("undefined" === typeof char || /\s/.test(char)) {
                    break;
                }
                endColumn = i + 2; // +2 because Monaco columns are 1-indexed
            }

            return {
                severity: monaco.MarkerSeverity.Error,
                startLineNumber: error.line,
                startColumn: startColumn,
                endLineNumber: error.line,
                endColumn: endColumn,
                message: error.message,
            };
        });

        monaco.editor.setModelMarkers(model, "sql-parser", markers);
    }, [editorProps.value, validateFn]);

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
