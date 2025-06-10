import SyntaxHighlighter from "react-syntax-highlighter";

import {Typography} from "antd";

import LogViewerLink from "./LogViewerLink";
import {highlighterCustomStyles} from "./utils";

import "highlight.js/styles/intellij-light.css";


const {Text} = Typography;

interface MessageProps {
    fileText: string;
    message: string;
    logEventIdx: number;
    streamId: string;
}

/**
 * Renders a message with syntax highlighting and a link to original file.
 *
 * @param props
 * @param props.fileText
 * @param props.logEventIdx
 * @param props.message
 * @param props.streamId
 * @return
 */
const Message = ({
    message,
    fileText,
    streamId,
    logEventIdx,
}: MessageProps) => {
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
            <LogViewerLink
                fileText={fileText}
                logEventIdx={logEventIdx}
                streamId={streamId}/>
        </>
    );
};

export default Message;
