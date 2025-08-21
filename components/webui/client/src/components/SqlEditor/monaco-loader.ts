/* eslint-disable import/default, @stylistic/max-len */
import {loader} from "@monaco-editor/react";
import * as monaco from "monaco-editor/esm/vs/editor/editor.api";
import EditorWorker from "monaco-editor/esm/vs/editor/editor.worker?worker";

import "monaco-editor/esm/vs/basic-languages/sql/sql.contribution.js";
import "monaco-editor/esm/vs/editor/contrib/clipboard/browser/clipboard.js";
import "monaco-editor/esm/vs/editor/contrib/contextmenu/browser/contextmenu.js";
import "monaco-editor/esm/vs/editor/contrib/find/browser/findController.js";
import "monaco-editor/esm/vs/editor/contrib/wordHighlighter/browser/wordHighlighter.js";
import "monaco-editor/esm/vs/editor/contrib/suggest/browser/suggestController.js";
import "monaco-editor/esm/vs/editor/contrib/placeholderText/browser/placeholderText.contribution.js";


/* eslint-enable import/default, @stylistic/max-len */


self.MonacoEnvironment = {
    /**
     * Creates a web worker for Monaco Editor.
     *
     * @return
     */
    getWorker () {
        return new EditorWorker();
    },
};

loader.config({monaco});
