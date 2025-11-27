import {
    Form,
    Input,
} from "antd";


/**
 * Renders a form item for inputting file paths for compression job submission.
 *
 * @return
 */
const PathsInputFormItem = () => (
    <Form.Item
        label={"Paths"}
        name={"paths"}
        rules={[{required: true, message: "Please enter at least one path"}]}
    >
        <Input.TextArea
            autoSize={{minRows: 4, maxRows: 10}}
            placeholder={"Enter paths to compress, one per line"}/>
    </Form.Item>
);


export default PathsInputFormItem;
