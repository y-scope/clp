import SyntaxHighlighter from "react-syntax-highlighter";

import {Typography} from "antd";

import LogViewerLink from "./LogViewerLink";
import {highlighterCustomStyles} from "./utils";
import { CLP_STORAGE_ENGINES, SETTINGS_STORAGE_ENGINE } from "../.././../../../config";
import { qtcreatorLight } from 'react-syntax-highlighter/dist/esm/styles/hljs';

const {Text} = Typography;

interface MessageProps {
    filePath: string;
    message: string;
    logEventIdx: number;
    streamId: string;
}

/**
 * Renders a message with syntax highlighting and a file path link.
 *
 * @param props
 * @param props.filePath
 * @param props.logEventIdx
 * @param props.message
 * @param props.streamId
 * @return
 */
const Message = ({
    message,
    filePath,
    streamId,
    logEventIdx,
}: MessageProps) => {
    return (
        <>
            {/* Parent `Text` component allows syntax highlighter to inherit AntD fonts. */}
            <Text>
                <SyntaxHighlighter
                    customStyle={highlighterCustomStyles}
                    language={
                        CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE
                            ? "json"
                            : "armasm"
                    }
                    style={qtcreatorLight}
                    wrapLongLines={true}
                >
                    {message}
                </SyntaxHighlighter>
            </Text>
            <LogViewerLink
                filePath={filePath}
                logEventIdx={logEventIdx}
                streamId={streamId}/>
        </>
    );
};

export default Message;
