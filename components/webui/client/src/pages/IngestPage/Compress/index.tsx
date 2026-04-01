import {
    useMutation,
    useQueryClient,
} from "@tanstack/react-query";
import {
    CLP_DEFAULT_DATASET_NAME,
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "@webui/common/config";
import {
    CompressionJobCreation,
    CompressionJobInputType,
    FsCompressionJobCreation,
    S3CompressionJobCreation,
} from "@webui/common/schemas/compression";
import {
    Form,
    Typography,
} from "antd";

import {submitCompressionJob} from "../../../api/compress";
import {DashboardCard} from "../../../components/DashboardCard";
import {
    SETTINGS_LOGS_INPUT_TYPE,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";
import ClpSFormItems from "./ClpSFormItems";
import PathsSelectFormItem from "./PathsSelectFormItem";
import S3InputFormItems from "./S3InputFormItems";
import SubmitFormItem from "./SubmitFormItem";


type FormValues = {
    // FS fields
    paths?: string[];

    // S3 fields
    bucket?: string;
    endpointUrl?: string;
    regionCode?: string;
    s3Paths?: string[];

    // Common CLP-S fields
    dataset?: string;
    timestampKey?: string;
    unstructured?: boolean;
};

const isS3Input = STORAGE_TYPE.S3 === SETTINGS_LOGS_INPUT_TYPE;

/**
 * Applies CLP-S fields to a compression job payload.
 *
 * @param payload The payload to modify.
 * @param values The form values.
 */
const applyClpSFields = (
    payload: FsCompressionJobCreation | S3CompressionJobCreation,
    values: FormValues,
) => {
    if ("undefined" === typeof values.dataset || 0 === values.dataset.length) {
        payload.dataset = CLP_DEFAULT_DATASET_NAME;
    } else {
        payload.dataset = values.dataset;
    }
    if (true === values.unstructured) {
        payload.unstructured = true;
    } else if ("undefined" !== typeof values.timestampKey) {
        payload.timestampKey = values.timestampKey;
    }
};

/**
 * Maps selected S3 tree paths to keyPrefix/keys fields on the payload.
 *
 * Selection semantics:
 * - If a single prefix (ending with "/") is selected: use `keyPrefix` mode (compress everything
 *   under it).
 * - If the bucket root ("") is selected: use `keyPrefix` with empty string (compress everything).
 * - Otherwise: use `keys` mode with the selected values as full object keys.
 *
 * @param s3Payload The payload to populate.
 * @param s3Paths Selected values from the S3 TreeSelect.
 */
const applyS3Paths = (
    s3Payload: S3CompressionJobCreation,
    s3Paths: string[],
) => {
    // Separate prefixes (ending with "/" or empty string for root) from object keys.
    const prefixes = s3Paths.filter((p) => p.endsWith("/") || "" === p);
    const objectKeys = s3Paths.filter((p) => false === p.endsWith("/") && "" !== p);

    if (0 < prefixes.length && 0 === objectKeys.length) {
        // Pure prefix selection. If single prefix, use keyPrefix mode.
        // If multiple prefixes, we still need to list objects under each — use the first
        // common ancestor as keyPrefix. For now, use the first prefix.
        s3Payload.keyPrefix = prefixes[0] ?? "";
    } else if (0 === prefixes.length && 0 < objectKeys.length) {
        // Pure object selection.
        s3Payload.keys = objectKeys;
    } else {
        // Mixed selection: expand prefixes into a key-based selection would require
        // server-side listing. For now, treat the entire selection as specific keys
        // for object entries, and use the first prefix as keyPrefix.
        if (0 < prefixes.length) {
            s3Payload.keyPrefix = prefixes[0] ?? "";
        }
        if (0 < objectKeys.length) {
            s3Payload.keys = objectKeys;
        }
    }
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
        let payload: CompressionJobCreation;

        if (isS3Input) {
            const s3Payload: S3CompressionJobCreation = {
                bucket: values.bucket as string,
                inputType: CompressionJobInputType.S3,
            };

            if (values.s3Paths && 0 < values.s3Paths.length) {
                applyS3Paths(s3Payload, values.s3Paths);
            }
            if ("undefined" !== typeof values.regionCode && 0 < values.regionCode.length) {
                s3Payload.regionCode = values.regionCode;
            }
            if ("undefined" !== typeof values.endpointUrl && 0 < values.endpointUrl.length) {
                s3Payload.endpointUrl = values.endpointUrl;
            }
            if (CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE) {
                applyClpSFields(s3Payload, values);
            }
            payload = s3Payload;
        } else {
            const fsPayload: FsCompressionJobCreation = {
                inputType: CompressionJobInputType.FS,
                paths: values.paths as string[],
            };

            if (CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE) {
                applyClpSFields(fsPayload, values);
            }
            payload = fsPayload;
        }

        mutate(payload);
    };

    return (
        <DashboardCard title={"Submit Compression Job"}>
            <Form
                form={form}
                layout={"vertical"}
                onFinish={handleSubmit}
            >
                {isS3Input ?
                    <S3InputFormItems/> :
                    <PathsSelectFormItem/>}
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
