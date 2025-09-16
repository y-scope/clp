import SyntaxHighlighter from "react-syntax-highlighter";
import {tomorrow} from "react-syntax-highlighter/dist/esm/styles/hljs";

import {Typography} from "antd";

import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../.././../../../config";
import LogViewerLink from "./LogViewerLink";
import {highlighterCustomStyles} from "./utils";


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
                    style={tomorrow}
                    wrapLongLines={true}
                    language={
                        CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE ?
                            "json" :
                            "armasm"
                    }
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
