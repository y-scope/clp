import {useEffect} from "react";

import {
    Editor,
    EditorProps,
    useMonaco,
} from "@monaco-editor/react";
import {language as sqlLanguage} from "monaco-editor/esm/vs/basic-languages/sql/sql.js";

import "./monaco-config";

type SqlEditorProps = Omit<EditorProps, "language">;

/**
 * Monaco editor with highlighting and autocomplete for SQL syntax.
 *
 * @param props
 * @return
 */
const SqlEditor = (props: SqlEditorProps) => {
    const monaco = useMonaco();


    useEffect(() => {
        if (null === monaco) {
            return () => {
            };
        }

        // Adds autocomplete suggestions for SQL keywords on editor load
        const provider = monaco.languages.registerCompletionItemProvider("sql", {
            provideCompletionItems: (model, position) => {
                const word = model.getWordUntilPosition(position);
                const range = {
                    startLineNumber: position.lineNumber,
                    endLineNumber: position.lineNumber,
                    startColumn: word.startColumn,
                    endColumn: word.endColumn,
                };
                const suggestions = sqlLanguage.keywords.map((keyword: string) => ({
                    detail: "SQL Keyword",
                    insertText: `${keyword} `,
                    kind: monaco.languages.CompletionItemKind.Keyword,
                    label: keyword,
                    range: range,
                }));

                return {suggestions: suggestions};
            },
        });

        return () => {
            provider.dispose();
        };
    }, [monaco]);

    return (
        <Editor
            language={"sql"}

            // Use white background while loading (default is grey) so transition to editor with
            // white background is less jarring.
            loading={<div style={{backgroundColor: "white", height: "100%", width: "100%"}}/>}
            options={{
                fontSize: 14,
                lineNumbers: "on",
                lineNumbersMinChars: 2,
                minimap: {enabled: false},
                overviewRulerBorder: false,
                placeholder: "Enter your SQL query",
                scrollBeyondLastLine: false,
                wordWrap: "on",
            }}
            {...props}/>
    );
};

export default SqlEditor;
