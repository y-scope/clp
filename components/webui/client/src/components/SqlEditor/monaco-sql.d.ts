declare module "monaco-editor/esm/vs/basic-languages/sql/sql.js" {
    import {languages} from "monaco-editor/esm/vs/editor/editor.api";


    interface SqlLanguageDefinition extends languages.IMonarchLanguage {
        keywords: string[];
    }

    export const language: SqlLanguageDefinition;
}
