import SyntaxHighlighter from "react-syntax-highlighter";
import {tomorrow} from "react-syntax-highlighter/dist/esm/styles/hljs";

import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {Typography} from "antd";

import {SETTINGS_STORAGE_ENGINE} from "../../../../../../config";
import LogViewerLink from "./LogViewerLink";
import {highlighterCustomStyles} from "./utils";


const {Text} = Typography;

interface MessageProps {
    archiveId: string;
    dataset: string;
    fileText: string;
    message: string;
    logEventIdx: number;
}

/**
 * Renders a message with syntax highlighting and a link to original file.
 *
 * @param props
 * @param props.archiveId
 * @param props.dataset
 * @param props.fileText
 * @param props.logEventIdx
 * @param props.message
 * @return
 */
const Message = ({
    archiveId,
    dataset,
    message,
    fileText,
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
            {CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE ?
                <LogViewerLink
                    archiveId={archiveId}
                    dataset={dataset}
                    fileText={fileText}
                    logEventIdx={logEventIdx}/> :
                <Text type={"secondary"}>
                    {fileText}
                </Text>}
        </>
    );
};

export default Message;
