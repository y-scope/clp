declare module "monaco-editor/esm/vs/basic-languages/sql/sql.js" {
    import {languages} from "monaco-editor";


    interface SqlLanguageDefinition extends languages.IMonarchLanguage {
        keywords: string[];
    }

    export const language: SqlLanguageDefinition;
}
