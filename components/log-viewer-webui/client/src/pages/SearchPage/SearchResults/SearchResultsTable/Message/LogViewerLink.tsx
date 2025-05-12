import {LinkOutlined} from "@ant-design/icons";
import {
    Tooltip,
    Typography,
} from "antd";

import styles from "./index.module.css";


const {Link} = Typography;

// eslint-disable-next-line no-warning-comments
// TODO: Fix link to connect to package log viewer when log viewer setup finished. Also pass
// proper args to package log viewer.
const LOG_VIEWER_URL = "https://y-scope.github.io/yscope-log-viewer/";

interface LogViewerLinkProps {
    filePath: string;
}

/**
 * Render a link to the log viewer with open file.
 *
 * @param props
 * @param props.filePath
 * @return
 */
const LogViewerLink = ({filePath}: LogViewerLinkProps) => (
    <Tooltip title={"Open file"}>
        <Link
            href={LOG_VIEWER_URL}
            target={"_blank"}
            type={"secondary"}
        >
            <LinkOutlined className={styles["linkIcon"] || ""}/>
            {filePath}
        </Link>
    </Tooltip>
);

export default LogViewerLink;
