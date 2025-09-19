import {CLP_DEFAULT_DATASET_NAME} from "@webui/common/config";
import {
    Form,
    Input,
} from "antd";


const DATASET_HELPER_TEXT = "The dataset that the archives belong to. If left empty, dataset " +
    "\"default\" is used.";
const TIMESTAMP_KEY_HELPER_TEXT = "The path (e.g. x.y) for the field containing the log event's" +
    " timestamp.";
const TIMESTAMP_KEY_HELPER_TEXT_EXTRA = "If not provided, events will not have assigned" +
    " timestamps and can only be searched from the command line without a timestamp filter.";


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
            <Input
                defaultValue={CLP_DEFAULT_DATASET_NAME}
                placeholder={DATASET_HELPER_TEXT}/>
        </Form.Item>
        <Form.Item
            label={"Timestamp Key"}
            name={"timestampKey"}
            tooltip={`${TIMESTAMP_KEY_HELPER_TEXT} ${TIMESTAMP_KEY_HELPER_TEXT_EXTRA}`}
        >
            <Input placeholder={TIMESTAMP_KEY_HELPER_TEXT}/>
        </Form.Item>
    </>
);


export default ClpSFormItems;
