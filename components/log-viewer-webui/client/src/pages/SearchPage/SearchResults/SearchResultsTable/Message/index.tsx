import { useMemo } from "react";
import hljs from "highlight.js/lib/core";
import armasm from "highlight.js/lib/languages/armasm";
import "highlight.js/styles/intellij-light.css";

import { Typography } from "antd";
import LogViewerLink from "./LogViewerLink";

const { Paragraph } = Typography;

interface MessageProps {
    message: string;
    filePath: string;
}

// Register the language once (outside component is OK too)
hljs.registerLanguage("armasm", armasm);

/**
 * Renders a message with syntax highlighting and a file path link.
 *
 * @param props
 * @param props.message
 * @param props.filePath
 * @return
 */
const Message = ({ message, filePath }: MessageProps) => {
            const highlighted = useMemo(() => {
        const html = hljs.highlight(message, {
            language: "armasm",
            ignoreIllegals: true,
        }).value;

        // Convert newlines into inline spans with a space
        return html.replace(/\n/g, " "); // or <span>\n</span> if needed
    }, [message]);

    return (
        <>
            <style>
            {`
                .hljs span {
                display: inline !important;
                white-space: pre-wrap;
                }
            `}
            </style>
            <Paragraph
                ellipsis={{
                    rows: 2,
                }}
            >
                <span
                    className="hljs"
                    style={{
                        display: "inline !important",
                        background: "none",
                        border: "none",
                        fontFamily: "inherit",
                        margin: "0",
                        padding: "0",
                    }}
                    dangerouslySetInnerHTML={{ __html: highlighted }}
                />
            </Paragraph>
            <LogViewerLink filePath={filePath} />
        </>
    );
};

export default Message;