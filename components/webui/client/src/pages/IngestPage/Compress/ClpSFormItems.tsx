import {CLP_DEFAULT_DATASET_NAME} from "@webui/common/config";
import {
    Checkbox,
    Form,
    Input,
    Typography,
} from "antd";

import styles from "./index.module.css";
import {validateDatasetName} from "./validation";


const DATASET_HELPER_TEXT = `If left empty, dataset "${CLP_DEFAULT_DATASET_NAME}" will be used.`;
const DATASET_PLACEHOLDER_TEXT = "The dataset for new archives";
const TIMESTAMP_KEY_HELPER_TEXT = (
    <>
        If not provided, events will not have assigned timestamps and can only be searched from
        the command line without a timestamp filter. Certain characters require escaping. See the
        {" "}
        <Typography.Link
            href={"https://docs.yscope.com/clp/v0.9.0/user-docs/reference-json-search-syntax.html#characters-that-require-escaping"}
            rel={"noopener"}
            target={"_blank"}
        >
            JSON search syntax docs
        </Typography.Link>
        . This field is ignored when &quot;Convert to JSON&quot; is enabled.
    </>
);
const TIMESTAMP_KEY_PLACEHOLDER_TEXT =
    "The path (e.g. x.y) for the field containing the log event's timestamp";
const UNSTRUCTURED_HELPER_TEXT = (
    <>
        Enable this for non-JSON logs. Each log event will be parsed and converted to JSON with
        {" "}
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
        {" "}
        fields. See the
        {" "}
        <Typography.Link
            href={"https://docs.yscope.com/clp/v0.9.0/user-docs/quick-start/clp-json.html#compressing-unstructured-text-logs"}
            rel={"noopener"}
            target={"_blank"}
        >
            documentation
        </Typography.Link>
        {" "}
        for more details.
    </>
);


/**
 * Renders additional compression job submission form items for CLP-S storage engine.
 *
 * @return
 */
const ClpSFormItems = () => {
    const unstructured = Form.useWatch<boolean>("unstructured");

    return (
        <>
            <Form.Item
                label={"Dataset"}
                name={"dataset"}
                tooltip={DATASET_HELPER_TEXT}
                rules={[
                    {
                        // eslint-disable-next-line @typescript-eslint/require-await
                        validator: async (_, value: unknown) => {
                            const error = validateDatasetName(value as string);
                            if (error) {
                                throw new Error(error);
                            }
                        },
                    },
                ]}
            >
                <Input placeholder={DATASET_PLACEHOLDER_TEXT}/>
            </Form.Item>
            <Form.Item
                label={"Timestamp Key"}
                name={"timestampKey"}
                tooltip={TIMESTAMP_KEY_HELPER_TEXT}
            >
                <Input
                    disabled={true === unstructured}
                    placeholder={TIMESTAMP_KEY_PLACEHOLDER_TEXT}/>
            </Form.Item>
            <Form.Item
                label={"Unstructured Logs"}
                name={"unstructured"}
                tooltip={UNSTRUCTURED_HELPER_TEXT}
                valuePropName={"checked"}
            >
                <Checkbox>Convert to JSON</Checkbox>
            </Form.Item>
        </>
    );
};


export default ClpSFormItems;
