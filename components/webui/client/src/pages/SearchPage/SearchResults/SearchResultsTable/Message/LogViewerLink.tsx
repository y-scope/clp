import {Link} from "react-router";

import {LinkOutlined} from "@ant-design/icons";
import {
    theme,
    Tooltip,
    Typography,
} from "antd";

import {STREAM_TYPE} from "../../../../../config";
import useSearchStore from "../../../../SearchPage/SearchState";
import styles from "./index.module.css";


interface LogViewerLinkProps {
    fileText: string;
    logEventIdx: number;
    streamId: string;
}

/**
 * Render a link to the log viewer.
 *
 * @param props
 * @param props.fileText
 * @param props.logEventIdx
 * @param props.streamId
 * @return
 */
const LogViewerLink = ({
    fileText,
    logEventIdx,
    streamId,
}: LogViewerLinkProps) => {
    const {token} = theme.useToken();
    const nullableCachedDataset = useSearchStore((state) => state.cachedDataset);
    // eslint-disable-next-line no-warning-comments
    // TODO: URL search parameters can't be null, so we need a way to
    // handle the nullable dataset parameter.
    // For now, we'll use an empty string to represent null.
    const cachedDataset = null === nullableCachedDataset ?
        "" :
        nullableCachedDataset;

    return (
        <Tooltip title={"Open file"}>
            <Typography.Text>
                <Link
                    className={styles["linkIcon"] || ""}
                    style={{color: token.colorLink}}
                    target={"_blank"}
                    to={{
                        pathname: "/streamFile",
                        search:
                            `?type=${encodeURIComponent(STREAM_TYPE)}` +
                            `&streamId=${encodeURIComponent(streamId)}` +
                            `&logEventIdx=${encodeURIComponent(logEventIdx)}` +
                            `&dataset=${encodeURIComponent(cachedDataset)}`,
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
