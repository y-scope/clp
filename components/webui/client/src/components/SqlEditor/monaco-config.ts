import { loader } from '@monaco-editor/react';
import * as monaco from "monaco-editor/esm/vs/editor/editor.api.js";
import EditorWorker from 'monaco-editor/esm/vs/editor/editor.worker?worker';

self.MonacoEnvironment = {
    getWorker: () => {
        return new EditorWorker();
    },
};

loader.config({ monaco });
