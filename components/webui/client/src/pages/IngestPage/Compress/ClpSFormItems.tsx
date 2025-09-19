import {CLP_DEFAULT_DATASET_NAME} from "@webui/common/config";
import {
    Form,
    Input,
} from "antd";


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
        >
            <Input
                defaultValue={CLP_DEFAULT_DATASET_NAME}
                placeholder={"The dataset that the archives belong to (optional)"}/>
        </Form.Item>
        <Form.Item
            label={"Timestamp Key"}
            name={"timestampKey"}
        >
            <Input
                placeholder={
                    "The path for the field containing the log event's " +
                                    "timestamp (optional)"
                }/>
        </Form.Item>
    </>
);


export default ClpSFormItems;
