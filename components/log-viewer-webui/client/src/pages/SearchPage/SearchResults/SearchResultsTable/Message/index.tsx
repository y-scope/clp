import SyntaxHighlighter from "react-syntax-highlighter";

import {Typography} from "antd";

import LogViewerLink from "./LogViewerLink";
import {highlighterCustomStyles} from "./utils";

import "highlight.js/styles/intellij-light.css";


const {Text} = Typography;

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
const Message = ({message, filePath}: MessageProps) => {
    return (
        <>
            {/* Parent `Text` component allows syntax highlighter to inherit AntD fonts. */}
            <Text>
                <SyntaxHighlighter
                    customStyle={highlighterCustomStyles}
                    language={"armasm"}
                    useInlineStyles={false}
                    wrapLongLines={true}
                >
                    {message}
                </SyntaxHighlighter>
            </Text>
            <LogViewerLink filePath={filePath}/>
        </>
    );
};

export default Message;
