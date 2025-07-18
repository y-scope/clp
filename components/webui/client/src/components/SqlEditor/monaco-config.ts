/* eslint-disable import/default */
import {loader} from "@monaco-editor/react";
import * as monaco from "monaco-editor/esm/vs/editor/editor.api";
import EditorWorker from "monaco-editor/esm/vs/editor/editor.worker?worker";


self.MonacoEnvironment = {
    getWorker: () => {
        return new EditorWorker();
    },
};

loader.config({monaco});
