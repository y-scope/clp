import {
    useMutation,
    useQueryClient,
} from "@tanstack/react-query";
import {
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "@webui/common/config";
import {
    CompressionJobInputType,
    FsCompressionJobCreation,
} from "@webui/common/schemas/compression";
import {
    Form,
    Radio,
    Typography,
} from "antd";

import {
    submitCompressionJob,
    submitScannerJob,
} from "../../../api/compress";
import {DashboardCard} from "../../../components/DashboardCard";
import {
    SETTINGS_LOGS_INPUT_TYPE,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";
import ClpSFormItems from "./ClpSFormItems";
import {
    applyClpSFields,
    buildS3Payload,
} from "./jobHelpers";
import PathsSelectFormItem from "./PathsSelectFormItem";
import S3InputFormItems from "./S3InputFormItems";
import ScannerAdvancedFormItems from "./ScannerAdvancedFormItems";
import SubmitFormItem from "./SubmitFormItem";


type FormValues = {
    bucket?: string;
    bufferChannelCapacity?: number;
    bufferFlushThresholdBytes?: number;
    bufferTimeoutSec?: number;
    dataset?: string;
    ingestMode?: string;
    paths?: string[];
    regionCode?: string;
    s3Paths?: string[];
    scanningIntervalSec?: number;
    timestampKey?: string;
    unstructured?: boolean;
};

const isS3Input = STORAGE_TYPE.S3 === SETTINGS_LOGS_INPUT_TYPE;

const INGEST_MODE_ONE_TIME = "one-time";
const INGEST_MODE_SCANNER = "scanner";


type SubmitResult = {
    type: "compression";
    jobId: number;
} | {
    type: "scanner";
    jobIds: number[];
};

/**
 * Builds and submits the appropriate job based on form values.
 *
 * @param values
 * @return
 */
const submitJob = async (values: FormValues): Promise<SubmitResult> => {
    const isScanner = isS3Input && INGEST_MODE_SCANNER === values.ingestMode;

    if (isS3Input) {
        const payload = buildS3Payload({
            bucket: values.bucket as string,
            regionCode: values.regionCode as string,
            s3Paths: values.s3Paths,
            scanner: isScanner,
            values: values,
        });

        if (isScanner) {
            const result = await submitScannerJob(payload);

            return {jobIds: result.jobIds, type: "scanner"};
        }

        const result = await submitCompressionJob(payload);

        return {jobId: result.jobId, type: "compression"};
    }

    const fsPayload: FsCompressionJobCreation = {
        inputType: CompressionJobInputType.FS,
        paths: values.paths as string[],
    };

    if (CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE) {
        applyClpSFields(fsPayload, values);
    }

    const result = await submitCompressionJob(fsPayload);

    return {jobId: result.jobId, type: "compression"};
};

/**
 * Formats the success message for a submit result.
 *
 * @param result
 * @return
 */
const getSuccessMessage = (result: SubmitResult): string => {
    if ("scanner" === result.type) {
        const ids = result.jobIds.join(", ");
        return 1 === result.jobIds.length ?
            `Scanner job created with ID: ${ids}` :
            `Scanner jobs created with IDs: ${ids}`;
    }

    return `Compression job submitted with ID: ${result.jobId.toString()}`;
};

/**
 * Renders a compression job submission form.
 *
 * @return
 */
const Compress = () => {
    const [form] = Form.useForm<FormValues>();
    const ingestMode = Form.useWatch("ingestMode", form);
    const isScanner = isS3Input && INGEST_MODE_SCANNER === ingestMode;

    const queryClient = useQueryClient();
    const {
        mutate,
        isPending: isSubmitting,
        isSuccess,
        isError,
        data,
        error,
    } = useMutation({
        mutationFn: submitJob,
        onSettled: async () => {
            await queryClient.invalidateQueries({queryKey: ["jobs"]});
        },
        onSuccess: () => {
            form.resetFields();
        },
    });

    return (
        <DashboardCard title={"Submit Compression Job"}>
            <Form
                form={form}
                layout={"vertical"}
                onFinish={mutate}
            >
                {isS3Input && (
                    <Form.Item
                        initialValue={INGEST_MODE_ONE_TIME}
                        label={"Ingest Mode"}
                        name={"ingestMode"}
                        tooltip={
                            "Scanner: continuously polls S3 for new objects and compresses them" +
                            " automatically."
                        }
                    >
                        <Radio.Group>
                            <Radio.Button value={INGEST_MODE_ONE_TIME}>
                                One-time
                            </Radio.Button>
                            <Radio.Button value={INGEST_MODE_SCANNER}>
                                Scanner
                            </Radio.Button>
                        </Radio.Group>
                    </Form.Item>
                )}
                {isS3Input ?
                    <S3InputFormItems isScanner={isScanner}/> :
                    <PathsSelectFormItem/>}
                {CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE && <ClpSFormItems/>}
                {isScanner && <ScannerAdvancedFormItems/>}
                <br/>
                <SubmitFormItem isSubmitting={isSubmitting}/>

                {isSuccess && (
                    <Typography.Text type={"success"}>
                        {getSuccessMessage(data)}
                    </Typography.Text>
                )}
                {isError && (
                    <Typography.Text type={"danger"}>
                        {"Failed to submit job: "}
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
