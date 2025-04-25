import React from "react";
import { Tooltip, Typography } from "antd";
import { LinkOutlined } from "@ant-design/icons";

const { Link } = Typography;

interface FilePathProps {
    filePath: string;
}

/**
 * A component to render a file path as a clickable link.
 *
 * @param props
 * @param props.filePath - The file path to display and link to.
 * @return JSX.Element
 */
const FilePath = ({ filePath }: FilePathProps) => (
    <Tooltip title="Open file">
        <Link href={`https://yscope.com/${filePath}`} target="_blank" type="secondary">
            <LinkOutlined style={{ marginRight: "4px" }} />
            {filePath}
        </Link>
    </Tooltip>
);

export default FilePath;
