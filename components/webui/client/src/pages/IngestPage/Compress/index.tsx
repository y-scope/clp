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
    Col,
    Form,
    Input,
    Radio,
    Row,
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
    LOGS_TYPE_HELPER_TEXT,
    TIMESTAMP_KEY_HELPER_TEXT,
} from "./formConstants";
import {
    applyClpSFields,
    buildS3Payload,
    filterValidS3Paths,
    getSuccessMessage,
    type SubmitResult,
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
const showClpSFields = CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE;
const TIMESTAMP_KEY_PLACEHOLDER =
    "The path (e.g. x.y) for the field containing the log event's timestamp";
const RADIO_BUTTON_STYLE = {width: "50%", textAlign: "center" as const};

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
            s3Paths: filterValidS3Paths(values.s3Paths),
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

    if (showClpSFields) {
        applyClpSFields(fsPayload, values);
    }

    const result = await submitCompressionJob(fsPayload);

    return {jobId: result.jobId, type: "compression"};
};

/**
 * Renders submit result feedback.
 *
 * @param isSuccess
 * @param isError
 * @param data
 * @param error
 * @return
 */
const renderFeedback = (
    isSuccess: boolean,
    isError: boolean,
    data: SubmitResult | undefined,
    error: Error | null,
) => {
    if (isSuccess) {
        return (
            <Typography.Text type={"success"}>
                {getSuccessMessage(data as SubmitResult)}
            </Typography.Text>
        );
    }
    if (isError) {
        return (
            <Typography.Text type={"danger"}>
                {"Failed to submit job: "}
                {error instanceof Error ?
                    error.message :
                    "Unknown error"}
            </Typography.Text>
        );
    }

    return null;
};

/**
 * Renders a compression job submission form.
 *
 * @return
 */
const Compress = () => {
    const [form] = Form.useForm<FormValues>();
    const ingestMode = Form.useWatch("ingestMode", form);
    const unstructured = Form.useWatch<boolean>("unstructured", form);
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
                requiredMark={"optional"}
                onFinish={mutate}
            >
                {isS3Input && (
                    <Row gutter={8}>
                        <Col span={5}>
                            <Form.Item
                                initialValue={INGEST_MODE_ONE_TIME}
                                label={"Job Type"}
                                name={"ingestMode"}
                                required={true}
                                tooltip={
                                    "Scanner: continuously polls S3 for new objects and" +
                                    " compresses them automatically."
                                }
                            >
                                <Radio.Group style={{width: "100%"}}>
                                    <Radio.Button
                                        style={RADIO_BUTTON_STYLE}
                                        value={INGEST_MODE_ONE_TIME}
                                    >
                                        One-time
                                    </Radio.Button>
                                    <Radio.Button
                                        style={RADIO_BUTTON_STYLE}
                                        value={INGEST_MODE_SCANNER}
                                    >
                                        Scanner
                                    </Radio.Button>
                                </Radio.Group>
                            </Form.Item>
                        </Col>
                    </Row>
                )}
                {showClpSFields && (
                    <Row gutter={8}>
                        <Col span={5}>
                            <Form.Item
                                initialValue={false}
                                label={"Logs Type"}
                                name={"unstructured"}
                                required={true}
                                tooltip={LOGS_TYPE_HELPER_TEXT}
                            >
                                <Radio.Group style={{width: "100%"}}>
                                    <Radio.Button
                                        style={RADIO_BUTTON_STYLE}
                                        value={false}
                                    >
                                        JSON
                                    </Radio.Button>
                                    <Radio.Button
                                        style={RADIO_BUTTON_STYLE}
                                        value={true}
                                    >
                                        Text
                                    </Radio.Button>
                                </Radio.Group>
                            </Form.Item>
                        </Col>
                        <Col span={19}>
                            <Form.Item
                                label={"Timestamp Key"}
                                name={"timestampKey"}
                                tooltip={TIMESTAMP_KEY_HELPER_TEXT}
                            >
                                <Input
                                    disabled={unstructured}
                                    placeholder={TIMESTAMP_KEY_PLACEHOLDER}/>
                            </Form.Item>
                        </Col>
                    </Row>
                )}
                {isS3Input ?
                    <S3InputFormItems isScanner={isScanner}/> :
                    <PathsSelectFormItem/>}
                {showClpSFields && <ClpSFormItems/>}
                {isScanner && <ScannerAdvancedFormItems/>}
                <SubmitFormItem isSubmitting={isSubmitting}/>
                {renderFeedback(isSuccess, isError, data, error)}
            </Form>
        </DashboardCard>
    );
};

export default Compress;
