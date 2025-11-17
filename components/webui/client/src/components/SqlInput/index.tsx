// Reference: https://github.com/vikyd/vue-monaco-singleline
import {
    useCallback,
    useEffect,
    useRef,
} from "react";

import {Nullable} from "@webui/common/utility-types";
import * as monaco from "monaco-editor/esm/vs/editor/editor.api.js";

import {ValidationError} from "../../sql-parser";
import SqlEditor, {
    SqlEditorProps,
    SqlEditorType,
} from "../SqlEditor";
import {escapeHoverMarkdown} from "./utils";


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
    const editorRef = useRef<Nullable<SqlEditorType>>(null);
    const decorationsRef = useRef<Nullable<monaco.editor.IEditorDecorationsCollection>>(null);

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
        if (null === editor) {
            return;
        }

        if (null === decorationsRef.current) {
            decorationsRef.current = editor.createDecorationsCollection();
        }

        const value = editorProps.value ?? "";

        if ("undefined" === typeof validateFn || "" === value.trim()) {
            decorationsRef.current.clear();

            return;
        }

        const errors = validateFn(value);
        const decorations: monaco.editor.IModelDeltaDecoration[] = errors.map((error) => ({
            range: new monaco.Range(error.line, error.startColumn, error.line, error.endColumn),
            options: {
                className: "squiggly-error",
                hoverMessage: {value: escapeHoverMarkdown(error.message), isTrusted: false},
                minimap: {
                    color: {id: "minimap.errorHighlight"},
                    position: monaco.editor.MinimapPosition.Inline,
                },

                overviewRuler: {
                    color: {id: "editorOverviewRuler.errorForeground"},
                    position: monaco.editor.OverviewRulerLane.Right,
                },
                showIfCollapsed: true,
                stickiness: monaco.editor.TrackedRangeStickiness.NeverGrowsWhenTypingAtEdges,
                zIndex: 30,
            },
        }
        ));

        decorationsRef.current.set(decorations);
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
                quickSuggestions: false,
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
