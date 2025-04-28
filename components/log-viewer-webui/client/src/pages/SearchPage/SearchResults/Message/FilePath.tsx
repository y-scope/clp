import { Tooltip, Typography } from "antd";
import { LinkOutlined } from "@ant-design/icons";
import styles from "./index.module.css"; // Import the CSS module

const { Link } = Typography;

// eslint-disable-next-line no-warning-comments
// TODO: Fix link to connect when log viewer setup with new webui. Also pass
// proper args to log viewer.
const LOG_VIEWER_URL = "https://yscope.com/";

interface FilePathProps {
    filePath: string;
}

/**
 * Render a file path as a clickable link.
 *
 * @param props
 * @param props.filePath
 * @return
 */
const FilePath = ({ filePath }: FilePathProps) => (
    <Tooltip title="Open file">
        <Link href={LOG_VIEWER_URL} target="_blank" type="secondary">
            <LinkOutlined className={styles['linkIcon'] || ""} />
            {filePath}
        </Link>
    </Tooltip>
);

export default FilePath;
