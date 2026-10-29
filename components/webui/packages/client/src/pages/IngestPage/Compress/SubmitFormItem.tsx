import {
    Button,
    Form,
} from "antd";


interface SubmitFormItemProps {
    isSubmitting: boolean;
}

/**
 * Render a submit button for a form.
 *
 * @param props
 * @param props.isSubmitting
 * @return
 */
const SubmitFormItem = ({isSubmitting}: SubmitFormItemProps) => (
    <Form.Item>
        <Button
            htmlType={"submit"}
            loading={isSubmitting}
            type={"primary"}
        >
            Submit
        </Button>
    </Form.Item>
);


export default SubmitFormItem;
