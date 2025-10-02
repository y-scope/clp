import {
    useMutation,
    useQueryClient,
} from "@tanstack/react-query";
import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {CompressionJobCreation} from "@webui/common/schemas/compression";
import {
    Form,
    Typography,
} from "antd";

import {submitCompressionJob} from "../../../api/compress";
import {DashboardCard} from "../../../components/DashboardCard";
import {SETTINGS_STORAGE_ENGINE} from "../../../config";
import ClpSFormItems from "./ClpSFormItems";
import PathsInputFormItem from "./PathsInputFormItem";
import SubmitFormItem from "./SubmitFormItem";


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

    const queryClient = useQueryClient();
    const {
        mutate,
        isPending: isSubmitting,
        isSuccess,
        isError,
        data,
        error,
    } = useMutation({
        mutationFn: submitCompressionJob,
        onSettled: async () => {
            // Invalidate queries that are affected by a new compression job.
            await queryClient.invalidateQueries({queryKey: ["jobs"]});
        },
        onSuccess: () => {
            form.resetFields();
        },
    });

    const handleSubmit = (values: FormValues) => {
        // eslint-disable-next-line no-warning-comments
        // TODO: replace the UI with a file selector and remove below string manipulation.
        // Convert multiline input to array of paths.
        const paths = values.paths
            .split("\n")
            .map((path) => path.trim())
            .filter((path) => 0 < path.length);

        const payload: CompressionJobCreation = {paths};

        if ("undefined" !== typeof values.dataset) {
            payload.dataset = values.dataset;
        }
        if ("undefined" !== typeof values.timestampKey) {
            payload.timestampKey = values.timestampKey;
        }

        mutate(payload);
    };

    return (
        <DashboardCard title={"Start Ingestion"}>
            <Form
                form={form}
                layout={"vertical"}
                onFinish={handleSubmit}
            >
                <PathsInputFormItem/>
                {CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE && <ClpSFormItems/>}
                <SubmitFormItem isSubmitting={isSubmitting}/>

                {isSuccess && (
                    <Typography.Text type={"success"}>
                        Compression job submitted successfully with ID:
                        {" "}
                        {data.jobId}
                    </Typography.Text>
                )}
                {isError && (
                    <Typography.Text type={"danger"}>
                        Failed to submit compression job:
                        {" "}
                        {error instanceof Error ?
                            error.message :
                            "Unknown error"}
                    </Typography.Text>
                )}
            </Form>
        </DashboardCard>
    );
};


export default Compress;
