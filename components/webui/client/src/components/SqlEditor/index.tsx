import {
    useCallback,
    useEffect,
    useState,
} from "react";

import {
    Editor,
    EditorProps,
    useMonaco,
} from "@monaco-editor/react";
import {language as sqlLanguage} from "monaco-editor/esm/vs/basic-languages/sql/sql.js";
import * as monaco from "monaco-editor/esm/vs/editor/editor.api.js";

import "./monaco-loader";


const MAX_VISIBLE_LINES: number = 5;

type SqlEditorProps = Omit<EditorProps, "language">;

/**
 * Monaco editor with highlighting and autocomplete for SQL syntax.
 *
 * @param props
 * @return
 */
const SqlEditor = (props: SqlEditorProps) => {
    const monacoEditor = useMonaco();

    useEffect(() => {
        if (null === monacoEditor) {
            return () => {
            };
        }

        // Adds autocomplete suggestions for SQL keywords on editor load
        const provider = monacoEditor.languages.registerCompletionItemProvider("sql", {
            provideCompletionItems: (model, position) => {
                const word = model.getWordUntilPosition(position);
                const range = {
                    startLineNumber: position.lineNumber,
                    endLineNumber: position.lineNumber,
                    startColumn: word.startColumn,
                    endColumn: word.endColumn,
                };
                const suggestions = sqlLanguage.keywords.map((keyword: string) => ({
                    detail: "Presto SQL (CLP)",
                    insertText: `${keyword} `,
                    kind: monacoEditor.languages.CompletionItemKind.Keyword,
                    label: keyword,
                    range: range,
                }));

                // When SQL keyword suggestions appear (e.g., after "SELECT a"), hitting Enter
                // accepts the first suggestion. To prevent accidental auto-completion
                // in multi-line queries and to allow users to dismiss suggestions more easily,
                // we make the current input the first suggestion.
                // Users can then use arrow keys to select a keyword if needed.
                const typedWord = model.getValueInRange(range);
                if (0 < typedWord.length) {
                    suggestions.push({
                        detail: "Current",
                        insertText: `${typedWord}\n`,
                        kind: monaco.languages.CompletionItemKind.Text,
                        label: typedWord,
                        range: range,
                    });
                }

                return {
                    suggestions: suggestions,
                    incomplete: true,
                };
            },
            triggerCharacters: [
                " ",
                "\n",
            ],
        });

        return () => {
            provider.dispose();
        };
    }, [monacoEditor]);

    const [isContentMultiline, setIsContentMultiline] = useState<boolean>(false);

    const handleMonacoMount = useCallback((editor: monaco.editor.IStandaloneCodeEditor) => {
        editor.onDidContentSizeChange((ev) => {
            if (false === ev.contentHeightChanged) {
                return;
            }
            if (null === monacoEditor) {
                throw new Error("Unexpected null Monaco instance");
            }
            const domNode = editor.getDomNode();
            if (null === domNode) {
                throw new Error("Unexpected null editor DOM node");
            }
            const model = editor.getModel();
            if (null === model) {
                throw new Error("Unexpected null editor model");
            }
            const lineHeight = editor.getOption(monacoEditor.editor.EditorOption.lineHeight);
            const contentHeight = editor.getContentHeight();
            const approxWrappedLines = Math.round(contentHeight / lineHeight);
            setIsContentMultiline(1 < approxWrappedLines);
            if (MAX_VISIBLE_LINES >= approxWrappedLines) {
                domNode.style.height = `${contentHeight}px`;
            } else {
                domNode.style.height = `${lineHeight * MAX_VISIBLE_LINES}px`;
            }
        });
    }, [monacoEditor]);

    return (
        <Editor
            language={"sql"}

            // Use white background while loading (default is grey) so transition to editor with
            // white background is less jarring.
            loading={<div style={{backgroundColor: "white", height: "100%", width: "100%"}}/>}
            options={{
                automaticLayout: true,
                folding: isContentMultiline,
                fontSize: 20,
                lineHeight: 30,
                lineNumbers: isContentMultiline ?
                    "on" :
                    "off",
                lineNumbersMinChars: 2,
                minimap: {enabled: false},
                overviewRulerBorder: false,
                placeholder: "Enter your SQL query",
                renderLineHighlightOnlyWhenFocus: true,
                scrollBeyondLastLine: false,
                wordWrap: "on",
            }}
            onMount={handleMonacoMount}
            {...props}/>
    );
};

export default SqlEditor;
