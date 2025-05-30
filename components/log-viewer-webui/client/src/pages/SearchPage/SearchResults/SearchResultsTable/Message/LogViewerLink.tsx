import {LinkOutlined} from "@ant-design/icons";
import {
    Tooltip,
} from "antd";
import { Link } from "react-router";
//import { Typography } from "antd";
import styles from "./index.module.css";
import { STREAM_TYPE } from "../utils"

interface LogViewerLinkProps {
    filePath: string;
    streamId: string;
    logEventIx: number;
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
const LogViewerLink = ({filePath, streamId, logEventIx}: LogViewerLinkProps) => (
    <Tooltip title={"Open file"}>
            <Link
                to={{
                    pathname: "/streamFile",
                    search:
                        `?type=${encodeURIComponent(STREAM_TYPE)}` +
                        `&streamId=${encodeURIComponent(streamId)}` +
                        `&logEventIdx=${encodeURIComponent(logEventIx)}`
                }}
                className={styles["linkIcon"] || ""}
                target={"_blank"}
            >
                <LinkOutlined />
                {filePath}
            </Link>
    </Tooltip>
);

export default LogViewerLink;
