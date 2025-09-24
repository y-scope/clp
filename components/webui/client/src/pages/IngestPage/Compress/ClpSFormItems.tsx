import {CLP_DEFAULT_DATASET_NAME} from "@webui/common/config";
import {
    Form,
    Input,
} from "antd";


const DATASET_HELPER_TEXT = "The dataset that the archives belong to. If left empty, dataset " +
    `"${CLP_DEFAULT_DATASET_NAME}" will be used.`;
const DATASET_PLACEHOLDER_TEXT = "The dataset for new archives";
const TIMESTAMP_KEY_HELPER_TEXT = "If not provided, events will not have assigned" +
    " timestamps and can only be searched from the command line without a timestamp filter.";
const TIMESTAMP_KEY_PLACEHOLDER_TEXT = "The path (e.g. x.y) for the field containing the log event's" +
    " timestamp.";

/**
 * Renders additional compression job submission form items for CLP-S storage engine.
 *
 * @return
 */
const ClpSFormItems = () => (
    <>
        <Form.Item
            label={"Dataset"}
            name={"dataset"}
            tooltip={DATASET_HELPER_TEXT}
        >
            <Input placeholder={DATASET_PLACEHOLDER_TEXT}/>
        </Form.Item>
        <Form.Item
            label={"Timestamp Key"}
            name={"timestampKey"}
            tooltip={TIMESTAMP_KEY_HELPER_TEXT}
        >
            <Input placeholder={TIMESTAMP_KEY_PLACEHOLDER_TEXT}/>
        </Form.Item>
    </>
);


export default ClpSFormItems;
