import {
    useCallback,
    useEffect,
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


type SqlEditorType = monaco.editor.IStandaloneCodeEditor;

type SqlEditorProps = Omit<EditorProps, "language"> & {
    disabled: boolean;

    /** Callback when the editor is mounted and ref is ready to use. */
    onEditorReady?: (editor: SqlEditorType) => void;
};

/**
 * Monaco editor with highlighting for SQL syntax.
 *
 * @param props
 * @return
 */
const SqlEditor = (props: SqlEditorProps) => {
    const {disabled, onEditorReady, ...editorProps} = props;
    const monacoEditor = useMonaco();
    const {token} = theme.useToken();

    const handleEditorDidMount = useCallback((
        editor: SqlEditorType,
    ) => {
        onEditorReady?.(editor);
    }, [onEditorReady]);

    // Define default and disabled themes for monaco editor
    useEffect(() => {
        if (null === monacoEditor) {
            return;
        }

        monacoEditor.editor.defineTheme("default-theme", {
            base: "vs",
            inherit: true,
            rules: [],
            colors: {
                "editor.background": color(token.colorBgContainer).hexa(),
                "editor.foreground": color(token.colorText).hexa(),
                "focusBorder": "#0000",
            },
        });

        monacoEditor.editor.defineTheme("disabled-theme", {
            base: "vs",
            inherit: true,
            rules: [],
            colors: {
                "editor.background": color(token.colorBgContainerDisabled).hexa(),
                "editor.foreground": color(token.colorTextDisabled).hexa(),
                "focusBorder": "#0000",
            },
        });
    }, [
        monacoEditor,
        token,
    ]);

    return (
        <div
            style={{
                border: `1px solid ${token.colorBorder}`,
                borderRadius: token.borderRadius,
                pointerEvents: disabled ?
                    "none" :
                    "auto",
            }}
        >
            <Editor
                language={"sql"}
                loading={
                    <div
                        style={{
                            backgroundColor: token.colorBgContainer,
                            height: "100%",
                            width: "100%",
                        }}/>
                }
                theme={disabled ?
                    "disabled-theme" :
                    "default-theme"}
                onMount={handleEditorDidMount}
                {...editorProps}/>
        </div>
    );
};

export default SqlEditor;
export type {
    SqlEditorProps,
    SqlEditorType,
};
