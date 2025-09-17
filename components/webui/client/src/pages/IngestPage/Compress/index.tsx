import {useState} from "react";

import {CompressionJobSchemaStatic} from "@webui/common/schemas/compression";
import {
    Button,
    Form,
    Input,
    message,
    Typography,
} from "antd";

import {useSubmitCompressionJob} from "../../../api/compress";
import {DashboardCard} from "../../../components/DashboardCard";
import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";


type FormValues = {
    paths: string;
    dataset?: string;
    timestampKey?: string;
};

/**
 * Renders a compression job submission form.
 *
 * @return
 */
const Compress = () => {
    const [form] = Form.useForm<FormValues>();
    const [submitResult, setSubmitResult] = useState<{
        success: boolean;
        message: string;
    } | null>(null);

    const [messageApi, contextHolder] = message.useMessage();
    const {
        mutate: submitCompressionJob,
        isPending: isSubmitting,
    } = useSubmitCompressionJob();

    const handleSubmit = (values: FormValues) => {
        setSubmitResult(null);

        // eslint-disable-next-line no-warning-comments
        // TODO: replace the UI with a file selector and remove below string manipulation.
        // Convert multiline input to array of paths.
        const paths = values.paths
            .split("\n")
            .map((path) => path.trim())
            .filter((path) => 0 < path.length);

        const payload: CompressionJobSchemaStatic = {
            paths: paths,
        };

        if ("undefined" !== typeof values.dataset) {
            payload.dataset = values.dataset;
        }

        if ("undefined" !== typeof values.timestampKey) {
            payload.timestampKey = values.timestampKey;
        }

        submitCompressionJob(payload, {
            onSuccess: (jobId) => {
                setSubmitResult({
                    success: true,
                    message: `Compression job submitted successfully with ID: ${jobId}`,
                });
                form.resetFields();
            },
            onError: (error: unknown) => {
                const errorMessage = error instanceof Error ?
                    error.message :
                    "Unknown error";

                setSubmitResult({
                    success: false,
                    message: `Failed to submit compression job: ${errorMessage}`,
                });
                messageApi.error(`Failed to submit compression job: ${errorMessage}`);
            },
        });
    };

    return (
        <DashboardCard title={"Start Ingestion"}>
            {contextHolder}
            <Form
                form={form}
                layout={"vertical"}
                onFinish={(values) => {
                    handleSubmit(values);
                }}
            >
                <Form.Item
                    label={"Paths"}
                    name={"paths"}
                    rules={[{required: true, message: "Please enter at least one path"}]}
                >
                    <Input.TextArea
                        autoSize={{minRows: 4, maxRows: 10}}
                        placeholder={"Enter paths to compress, one per line"}/>
                </Form.Item>
                {CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE && (
                    <>
                        <Form.Item
                            label={"Dataset"}
                            name={"dataset"}
                        >
                            <Input
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
                )}

                <Form.Item>
                    <Button
                        htmlType={"submit"}
                        loading={isSubmitting}
                        type={"primary"}
                    >
                        {isSubmitting ?
                            "Submitting..." :
                            "Submit"}
                    </Button>
                </Form.Item>

                {submitResult && (
                    <Typography.Text
                        type={submitResult.success ?
                            "success" :
                            "danger"}
                    >
                        {submitResult.message}
                    </Typography.Text>
                )}
            </Form>
        </DashboardCard>
    );
};

export default Compress;
