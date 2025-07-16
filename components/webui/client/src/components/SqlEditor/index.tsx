import {Editor, useMonaco, EditorProps} from "@monaco-editor/react";
import {useEffect} from "react";
import { language as sqlLanguage } from 'monaco-editor/esm/vs/basic-languages/sql/sql.js';

interface SqlEditorProps extends Omit<EditorProps, 'language'> {}

/**
 * Monaco editor with highlighting and autocomplete for SQL syntax.
 */
const SqlEditor = (props: SqlEditorProps) => {
    const monaco = useMonaco();


    useEffect(() => {
        if (null === monaco) {
            return;
        }
        
        // Adds autocomplete suggestions for SQL keywords on editor load
        const provider = monaco.languages.registerCompletionItemProvider('sql', {
            provideCompletionItems: () => {
                const suggestions = sqlLanguage[`keywords`].map((keyword: string) => ({
                    label: keyword,
                    kind: monaco.languages.CompletionItemKind.Keyword,
                    insertText: keyword + ' ',
                    detail: 'SQL Keyword'
                }));

                return { suggestions };
            }
        });

        return () => {
            provider.dispose();
        };
    }, [monaco]);

    return (
        <Editor
            language="sql"
            // Use white background while loading (default is grey) so transition to editor with
            // white background is less jarring.
            loading={
                <div style={{ backgroundColor: 'white', height: '100%', width: '100%' }}/>
            }
            options={{
                minimap: { enabled: false },
                lineNumbers: "on",
                lineNumbersMinChars: 2,
                scrollBeyondLastLine: false,
                wordWrap: "on",
                placeholder: "Enter your SQL query",
                overviewRulerBorder: false,
                fontSize: 14,
            }}
            {...props}
        />
    );
};

export default SqlEditor;
