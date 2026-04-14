import {CLP_DEFAULT_DATASET_NAME} from "@webui/common/config";
import {
    Form,
    Input,
} from "antd";

import {validateDatasetName} from "./validation";


const DATASET_HELPER_TEXT = `If left empty, dataset "${CLP_DEFAULT_DATASET_NAME}" will be used.`;

const DATASET_PLACEHOLDER_TEXT = "The dataset for new archives";


/**
 * Renders the Dataset form item for CLP-S storage engine.
 *
 * @return
 */
const ClpSFormItems = () => (
    <Form.Item
        label={"Dataset"}
        name={"dataset"}
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
        tooltip={DATASET_HELPER_TEXT}
    >
        <Input placeholder={DATASET_PLACEHOLDER_TEXT}/>
    </Form.Item>
);


export default ClpSFormItems;
