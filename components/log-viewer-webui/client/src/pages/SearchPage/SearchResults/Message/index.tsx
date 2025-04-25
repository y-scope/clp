import React from "react";
import { Typography } from "antd";
import FilePath from "./FilePath";
import SyntaxHighlighter from "react-syntax-highlighter";
import "highlight.js/styles/intellij-light.css"; // Import the theme CSS

const { Text } = Typography;

interface MessageProps {
    message: string;
    filePath: string;
}

/**
 * A component to render a message with syntax highlighting and a file path link.
 *
 * @param props
 * @param props.message - The message to display with syntax highlighting.
 * @param props.filePath - The file path to display as a link.
 * @return JSX.Element
 */
const Message = ({ message, filePath }: MessageProps) => (
    <>
        <Text>
            <SyntaxHighlighter
                language="armasm"
                customStyle={{
                    background: "none",
                    padding: "0",
                    margin: "0",
                    fontFamily: "inherit",
                    border: "none",
                }}
                useInlineStyles={false}
            >
                {message}
            </SyntaxHighlighter>
        </Text>
        <FilePath filePath={filePath} />
    </>
);

export default Message;
