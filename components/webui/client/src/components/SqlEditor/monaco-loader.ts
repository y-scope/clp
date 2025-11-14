/* eslint-disable import/default */
import {loader} from "@monaco-editor/react";
import * as monaco from "monaco-editor/esm/vs/editor/editor.api";
import EditorWorker from "monaco-editor/esm/vs/editor/editor.worker?worker";

import "monaco-editor/esm/vs/basic-languages/sql/sql.contribution.js";
import "monaco-editor/esm/vs/editor/contrib/hover/browser/hoverContribution.js";
import "monaco-editor/esm/vs/editor/contrib/hover/browser/markerHoverParticipant.js";


/* eslint-enable import/default */


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
