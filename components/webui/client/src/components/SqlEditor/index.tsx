import {Editor} from "@monaco-editor/react";
import {useState} from "react";

/**
 * Renders a Monaco editor for SQL query input.
 *
 * @return
 */
const SqlEditor = () => {
    const [sqlQuery, setSqlQuery] = useState<string>("");

    const handleSqlQueryChange = (value: string | undefined) => {
        setSqlQuery(value || "");
    };

    return (
        <Editor
            height="200px"
            language="sql"
            value={sqlQuery}
            onChange={handleSqlQueryChange}
            options={{
                minimap: {enabled: false},
                lineNumbers: "on",
                scrollBeyondLastLine: false,
                wordWrap: "on",
            }}
        />
    );
};

export default SqlEditor;
