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
    streamId: string;
    logEventIdx: number;
}

/**
 * Render a link to the log viewer with open file.
 *
 * @param props
 * @param props.filePath
 * @param props.streamId
 * @param props.logEventIdx
 * @return
 */
const LogViewerLink = ({filePath, streamId, logEventIdx}: LogViewerLinkProps) => (
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
