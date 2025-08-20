import {
    useCallback,
    useEffect,
    useImperativeHandle,
    useRef,
    useState,
} from "react";

import EditorComponent, {
    Editor,
    EditorProps,
    useMonaco,
} from "@monaco-editor/react";
import {language as sqlLanguage} from "monaco-editor/esm/vs/basic-languages/sql/sql.js";
import * as monaco from "monaco-editor/esm/vs/editor/editor.api.js";
import {theme} from "antd";
import Color from "color";

import "./monaco-loader";


const MAX_VISIBLE_LINES: number = 5;

export type SqlEditorRef = {
    focus: () => void;
};

type SqlEditorProps = Omit<EditorProps, "language"> & React.RefAttributes<SqlEditorRef> & {
    disabled: boolean;
    onDidMount?: () => void;
};

/**
 * Monaco editor with highlighting and autocomplete for SQL syntax.
 *
 * @param props
 * @return
 */
const SqlEditor = (props: SqlEditorProps) => {
    const { ref, disabled, onDidMount, ...editorProps } = props;
    const monacoEditor = useMonaco();
    const { token } = theme.useToken();
    const editorRef = useRef<monaco.editor.IStandaloneCodeEditor>(null);

    useImperativeHandle(ref, () => ({
        focus: () => {
            console.log("Focusing SQL editor2");
            console.log("Editor ref:", editorRef);
            editorRef?.current?.focus()
        }
    }), []);

    const handleEditorDidMount = useCallback((
        editor: monaco.editor.IStandaloneCodeEditor,
    ) => {
        editorRef.current = editor;
        onDidMount?.();
        console.log("Editor mounted:", editor);
    }, [onDidMount]);

    console.log(token.colorBgContainerDisabled);

    // Define disabled theme when monaco is available
    useEffect(() => {
        if (monacoEditor) {
            monacoEditor.editor.defineTheme('disabled-theme', {
                base: 'vs',
                inherit: true,
                rules: [],
                colors: {
                    'editor.background': Color(token.colorBgContainerDisabled).hexa(),
                    'editor.foreground': Color(token.colorTextDisabled).hexa(),
                    'focusBorder': '#00000000', // transparent
                }
            });
        }
    }, [monacoEditor, token]);

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

    return (
        <div style={disabled ? { pointerEvents: 'none' } : {}}>
            <EditorComponent
                language={"sql"}
                theme={disabled ? 'disabled-theme' : 'light'}
                // Use white background while loading (default is grey) so transition to editor with
                // white background is less jarring.
                loading={<div style={{backgroundColor: "white", height: "100%", width: "100%"}}/>}
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
                    readOnly: disabled,
                }}
                onMount={handleEditorDidMount}
                {...editorProps}/>
        </div>
    );
};

export default SqlEditor;
