import {Link} from "react-router";

import {LinkOutlined} from "@ant-design/icons";
import {
    theme,
    Tooltip,
    Typography,
} from "antd";

import styles from "./index.module.css";


interface LogViewerLinkProps {
    archiveId: string;
    dataset: string;
    fileText: string;
    logEventIdx: number;
}

/**
 * Render a link to the log viewer.
 *
 * @param props
 * @param props.archiveId
 * @param props.dataset
 * @param props.fileText
 * @param props.logEventIdx
 * @return
 */
const LogViewerLink = ({
    archiveId,
    dataset,
    fileText,
    logEventIdx,
}: LogViewerLinkProps) => {
    const {token} = theme.useToken();

    const searchParams = new URLSearchParams({
        archiveId: encodeURIComponent(archiveId),
        logEventIdx: encodeURIComponent(logEventIdx),
    });

    if ("" !== dataset) {
        searchParams.append("dataset", dataset);
    }

    return (
        <Tooltip title={"Open file"}>
            <Typography.Text>
                <Link
                    className={styles["linkIcon"] || ""}
                    style={{color: token.colorLink}}
                    target={"_blank"}
                    to={{
                        pathname: "/streamFile",
                        search: `?${searchParams.toString()}`,
                    }}
                >
                    <LinkOutlined/>
                    {fileText}
                </Link>
            </Typography.Text>
        </Tooltip>
    );
};


export default LogViewerLink;
