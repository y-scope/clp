import { Typography } from "antd";
import FilePath from "./FilePath";
import SyntaxHighlighter from "react-syntax-highlighter";
import "highlight.js/styles/intellij-light.css";
import { highlighterCustomStyles } from "./utils";

const { Text } = Typography;

interface MessageProps {
    message: string;
    filePath: string;
}

/**
 * Renders a message with syntax highlighting and a file path link.
 *
 * @param props
 * @param props.message
 * @param props.filePath
 * @return
 */
const Message = ({ message, filePath }: MessageProps) => {
    return (
        <>
            {/* Parent `Text` component allows syntax highlighter to inherit AntD fonts. */}
            <Text>
                <SyntaxHighlighter
                    language="armasm"
                    customStyle={highlighterCustomStyles}
                    useInlineStyles={false}
                >
                    {message}
                </SyntaxHighlighter>
            </Text>
            <FilePath filePath={filePath} />
        </>
    );
};

export default Message;
