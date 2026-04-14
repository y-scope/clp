import {Typography} from "antd";

import styles from "./index.module.css";


const TIMESTAMP_KEY_HELPER_TEXT = (
    <>
        If not provided, events will not have assigned timestamps and can only be searched from
        the command line without a timestamp filter. Certain characters require escaping. See the
        {" "}
        <Typography.Link
            href={"https://docs.yscope.com/clp/main/user-docs/reference-json-search-syntax.html#characters-that-require-escaping"}
            rel={"noopener"}
            target={"_blank"}
        >
            JSON search syntax docs
        </Typography.Link>
        . This field is ignored when logs type is &quot;Text&quot;.
    </>
);

const LOGS_TYPE_HELPER_TEXT = (
    <>
        {"Select \u201cText\u201d for unstructured logs." +
        " Each log event will be parsed and converted to JSON with "}
        <Typography.Text
            className={styles["tooltipCode"] || ""}
            code={true}
        >
            timestamp
        </Typography.Text>
        {" and "}
        <Typography.Text
            className={styles["tooltipCode"] || ""}
            code={true}
        >
            message
        </Typography.Text>
        {" fields. See the "}
        <Typography.Link
            href={"https://docs.yscope.com/clp/main/user-docs/quick-start/clp-json.html#compressing-unstructured-text-logs"}
            rel={"noopener"}
            target={"_blank"}
        >
            documentation
        </Typography.Link>
        {" for more details."}
    </>
);

export {
    LOGS_TYPE_HELPER_TEXT,
    TIMESTAMP_KEY_HELPER_TEXT,
};
