import {
    useCallback,
    useEffect,
    useImperativeHandle,
    useRef,
} from "react";

import {
    Editor,
    EditorProps,
    useMonaco,
} from "@monaco-editor/react";
import {theme} from "antd";
import color from "color";
import {language as sqlLanguage} from "monaco-editor/esm/vs/basic-languages/sql/sql.js";
import * as monaco from "monaco-editor/esm/vs/editor/editor.api.js";

import "./monaco-loader";


type SqlEditorRef = {
    focus: () => void;
};

type SqlEditorProps = Omit<EditorProps, "language"> & React.RefAttributes<SqlEditorRef> & {
    disabled: boolean;

    /** Callback when the editor is mounted and ref is ready to use. */
    onEditorReady?: () => void;
};

/**
 * Monaco editor with highlighting and autocomplete for SQL syntax.
 *
 * @param props
 * @return
 */
const SqlEditor = (props: SqlEditorProps) => {
    const {ref, disabled, onEditorReady, ...editorProps} = props;
    const editorRef = useRef<monaco.editor.IStandaloneCodeEditor>(null);
    const monacoEditor = useMonaco();
    const {token} = theme.useToken();

    useImperativeHandle(ref, () => ({
        focus: () => {
            editorRef.current?.focus();
        },
    }), []);

    const handleEditorDidMount = useCallback((
        editor: monaco.editor.IStandaloneCodeEditor,
    ) => {
        editorRef.current = editor;
        onEditorReady?.();
    }, [onEditorReady]);

    // Define disabled theme for monaco editor
    useEffect(() => {
        if (null === monacoEditor) {
            return;
        }
        monacoEditor.editor.defineTheme("disabled-theme", {
            base: "vs",
            inherit: true,
            rules: [],
            colors: {
                "editor.background": color(token.colorBgContainerDisabled).hexa(),
                "editor.foreground": color(token.colorTextDisabled).hexa(),

                // transparent
                "focusBorder": "#00000000",
            },
        });
    }, [
        monacoEditor,
        token
    ]);

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
                // we make the current input the first suggestion. Users can then use arrow keys
                // to select a keyword if needed.
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
            ],
        });

        return () => {
            provider.dispose();
        };
    }, [monacoEditor]);

    return (
        <div
            style={
                disabled ?
                    {pointerEvents: "none"} :
                    {}
            }
        >
            <Editor
                language={"sql"}
                loading={
                    <div
                        style={{
                            backgroundColor: "white",
                            height: "100%",
                            width: "100%",
                        }}/>
                }
                options={{
                    automaticLayout: true,
                    fontSize: 20,
                    lineHeight: 30,
                    lineNumbers: "off",
                    minimap: {enabled: false},
                    overviewRulerBorder: false,
                    placeholder: "Enter your SQL query",
                    renderLineHighlightOnlyWhenFocus: true,
                    scrollBeyondLastLine: false,
                    wordWrap: "on",
                }}
                theme={disabled ?
                    "disabled-theme" :
                    "light"}
                onMount={handleEditorDidMount}
                {...editorProps}/>
        </div>
    );
};

export default SqlEditor;
export type {SqlEditorRef};
