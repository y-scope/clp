import {LinkOutlined} from "@ant-design/icons";
import {
    Tooltip,
} from "antd";
import { Link } from "react-router";
import styles from "./index.module.css";

interface LogViewerLinkProps {
    filePath: string;
    streamId: string;
    logEventIdx: number;
    streamType: string;
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
const LogViewerLink = ({filePath, streamId, logEventIdx, streamType}: LogViewerLinkProps) => (
    <Tooltip title={"Open file"}>
        <Link
            to={
                `/stream?type=${encodeURIComponent(streamType)}` +
                `&streamId=${encodeURIComponent(streamId)}` +
                `&logEventIdx=${encodeURIComponent(logEventIdx)}`
            }
            className={styles["linkIcon"] || ""}
            style={{ color: "inherit" }}
        >
            <LinkOutlined />
            {filePath}
        </Link>
    </Tooltip>
);

export default LogViewerLink;
