import {
    Form,
    Input,
} from "antd";

import {validateAbsolutePaths} from "./validation";


/**
 * Renders a form item for inputting file paths for compression job submission.
 *
 * @return
 */
const PathsInputFormItem = () => (
    <Form.Item
        label={"Paths"}
        name={"paths"}
        rules={[
            {
                message: "Please enter at least one path.",
                required: true,
                whitespace: true,
            },
            {
                // The `validator` function expects a `Promise` to be returned.
                // eslint-disable-next-line @typescript-eslint/require-await
                validator: async (_, value: string) => {
                    const error = validateAbsolutePaths(value);
                    if (null !== error) {
                        throw new Error(error);
                    }
                },
            },
        ]}
    >
        <Input.TextArea
            autoSize={{minRows: 4, maxRows: 10}}
            placeholder={"Enter absolute paths on the server host, one per line"}/>
    </Form.Item>
);


export default PathsInputFormItem;
