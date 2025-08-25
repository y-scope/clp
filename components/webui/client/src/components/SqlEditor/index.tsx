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
 * Monaco editor with highlighting for SQL syntax.
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
                    folding: false,
                    fontSize: 16,
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
