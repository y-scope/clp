import {Link} from "react-router";

import {LinkOutlined} from "@ant-design/icons";
import {
    Tooltip,
    Typography,
} from "antd";

import {STREAM_TYPE} from "../utils";
import styles from "./index.module.css";


interface LogViewerLinkProps {
    filePath: string;
    logEventIdx: number;
    streamId: string;
}

/**
 * Render a link to the log viewer with open file.
 *
 * @param props
 * @param props.filePath
 * @param props.logEventIdx
 * @param props.streamId
 * @return
 */
const LogViewerLink = ({
    filePath,
    logEventIdx,
    streamId,
}: LogViewerLinkProps) => (
    <Tooltip title={"Open file"}>
        <Typography.Link>
            <Link
                className={styles["linkIcon"] || ""}
                target={"_blank"}
                to={{
                    pathname: "/streamFile",
                    search:
                        `?type=${encodeURIComponent(STREAM_TYPE)}` +
                        `&streamId=${encodeURIComponent(streamId)}` +
                        `&logEventIdx=${encodeURIComponent(logEventIdx)}`,
                }}
            >
                <LinkOutlined/>
                {filePath}
            </Link>
        </Typography.Link>
    </Tooltip>
);

export default LogViewerLink;
