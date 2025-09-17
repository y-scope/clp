import {useState} from "react";

import {
    Button,
    Form,
    Input,
    Typography,
} from "antd";

import {
    type CompressionJobSchema,
    submitCompressionJob,
} from "../../../api/compress";
import {DashboardCard} from "../../../components/DashboardCard";
import {CLP_STORAGE_ENGINES, SETTINGS_STORAGE_ENGINE} from "../../../config";


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
    const [isSubmitting, setIsSubmitting] = useState<boolean>(false);
    const [submitResult, setSubmitResult] = useState<{
        success: boolean;
        message: string;
    } | null>(null);

    const handleSubmit = async (values: FormValues) => {
        setIsSubmitting(true);
        setSubmitResult(null);

        // Convert multiline input to array of paths
        const paths = values.paths
            .split("\n")
            .map((path) => path.trim())
            .filter((path) => 0 < path.length);

        try {
            const payload: CompressionJobSchema = {
                paths: paths,
            };

            if ("undefined" !== typeof values.dataset) {
                payload.dataset = values.dataset;
            }

            if ("undefined" !== typeof values.timestampKey) {
                payload.timestampKey = values.timestampKey;
            }

            const jobId = await submitCompressionJob(payload);

            setSubmitResult({
                success: true,
                message: `Compression job submitted successfully with ID: ${jobId}`,
            });
            form.resetFields();
        } catch (error: unknown) {
            setSubmitResult({
                success: false,
                message: `Failed to submit compression job: ${
                    error instanceof Error ?
                        error.message :
                        "Unknown error"
                }`,
            });
        } finally {
            setIsSubmitting(false);
        }
    };

    return (
        <DashboardCard title={"Start Ingestion"}>
            <Form
                form={form}
                layout={"vertical"}
                onFinish={(values) => {
                    handleSubmit(values).catch((error: unknown) => {
                        console.error("Error in handleSubmit:", error);
                    });
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
