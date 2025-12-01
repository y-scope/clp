import {Value} from "@sinclair/typebox/value";
import {AbsolutePathSchema} from "@webui/common/schemas/compression";
import {Nullable} from "@webui/common/utility-types";
import {
    Form,
    Input,
} from "antd";


/**
 * Validates that all non-empty lines in the input are absolute paths using AbsolutePathSchema.
 *
 * @param value The multiline string input containing paths.
 * @return An error message if validation fails, or null if validation passes.
 */
const validateAbsolutePaths = (value: string): Nullable<string> => {
    const lines = value.split("\n");
    let pathCount = 0;
    for (const [index, line] of lines.entries()) {
        const trimmedLine = line.trim();
        if (0 === trimmedLine.length) {
            continue;
        }
        if (false === Value.Check(AbsolutePathSchema, trimmedLine)) {
            return `Line ${index + 1}: Path must be absolute (start with "/").`;
        }
        pathCount++;
    }

    if (0 === pathCount) {
        return "Please enter at least one path.";
    }

    return null;
};

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
                // The `validator` function expects a `Promise` to be returned.
                // eslint-disable-next-line @typescript-eslint/require-await
                validator: async (_, value?: string) => {
                    const error = validateAbsolutePaths(value ?? "");
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
