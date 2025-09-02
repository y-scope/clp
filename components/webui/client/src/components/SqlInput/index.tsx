import {forwardRef, useCallback} from "react";
import SqlEditor, {SqlEditorRef} from "../SqlEditor";
import * as monaco from "monaco-editor/esm/vs/editor/editor.api";

type SqlInputProps = Omit<React.ComponentProps<typeof SqlEditor>, "height" | "options" | "language" | "onKeyDown" | "onPaste" | "onEditorReady"> & {
    /** Placeholder text for the input. */
    placeholder?: string;
};

/**
 * SqlInput: Monaco-based single-line SQL input.
 * Follows: https://farzadyz.me/blog/single-line-monaco-editor
 */
const SqlInput = forwardRef<SqlEditorRef, SqlInputProps>((props, ref) => {
    const {placeholder, ...rest} = props;

    // Prevent newlines in the editor
    const handleKeyDown = useCallback((e: any) => {
        if (e.key === "Enter" || e.key === "Return") {
            e.preventDefault();
            e.stopPropagation();
        }
    }, []);

    // Prevent pasting newlines
    const handlePaste = useCallback((e: any) => {
        if (e.clipboardData) {
            const text = e.clipboardData.getData("text/plain");
            if (text.includes("\n")) {
                e.preventDefault();
                // Only paste the first line
                const firstLine = text.split("\n")[0];
                document.execCommand("insertText", false, firstLine);
            }
        }
    }, []);

    // Enforce single line and add custom Enter action
    const handleEditorReady = useCallback((editor: monaco.editor.IStandaloneCodeEditor) => {
        // Prevent multi-line input by repositioning cursor and trimming value
        editor.onDidChangeCursorPosition((e) => {
            if (e.position.lineNumber > 1) {
                // Trim editor value
                editor.setValue(editor.getValue().replace(/\r?\n/g, " "));
                // Bring back the cursor to the end of the first line
                editor.setPosition({
                    lineNumber: 1,
                    column: Infinity,
                });
            }
        });

        // Add custom action for Enter key (form submission, currently a no-op)
        editor.addAction({
            id: "submitInSingleMode",
            label: "Submit in single mode",
            keybindings: [monaco.KeyCode.Enter],
            run: () => {
                // You can trigger form submit here if needed
            },
        });
    }, []);

    return (
        <SqlEditor
            ref={ref}
            height={32}
            options={{
                renderLineHighlight: "none",
                quickSuggestions: false,
                glyphMargin: false,
                lineDecorationsWidth: 0,
                folding: false,
                fixedOverflowWidgets: true,
                acceptSuggestionOnEnter: "on",
                hover: {delay: 100},
                roundedSelection: false,
                contextmenu: false,
                cursorStyle: "line-thin",
                occurrencesHighlight: false,
                links: false,
                minimap: {enabled: false},
                wordBasedSuggestions: false,
                find: {
                    addExtraSpaceOnTop: false,
                    autoFindInSelection: "never",
                    seedSearchStringFromSelection: "never",
                },
                fontSize: 14,
                fontWeight: "normal",
                wordWrap: "off",
                lineNumbers: "off",
                lineNumbersMinChars: 0,
                overviewRulerLanes: 0,
                overviewRulerBorder: false,
                hideCursorInOverviewRuler: true,
                scrollBeyondLastColumn: 0,
                scrollbar: {
                    horizontal: "hidden",
                    vertical: "hidden",
                    alwaysConsumeMouseWheel: false,
                },
                padding: {
                    top: 4,
                    bottom: 4,
                },
                placeholder: placeholder ?? "Enter SQL",
                scrollBeyondLastLine: false,
                lineHeight: 24,
            }}
            onKeyDown={handleKeyDown}
            onPaste={handlePaste}
            onEditorReady={handleEditorReady}
            {...rest}
        />
    );
});

export default SqlInput;
