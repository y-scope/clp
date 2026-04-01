import {
    Form,
    Input,
} from "antd";

import S3KeySelectFormItem from "./S3KeySelectFormItem";


const BUCKET_PLACEHOLDER_TEXT = "S3 bucket name (e.g. my-logs-bucket)";
const REGION_CODE_PLACEHOLDER_TEXT = "AWS region (e.g. us-east-1)";
const ENDPOINT_URL_PLACEHOLDER_TEXT = "Custom S3 endpoint (e.g. http://localhost:9000)";

const BUCKET_HELPER_TEXT = "The S3 bucket containing the logs to compress.";
const REGION_CODE_HELPER_TEXT =
    "The AWS region where the bucket is located." +
    " Required for standard AWS S3.";
const ENDPOINT_URL_HELPER_TEXT =
    "For S3-compatible storage (e.g. MinIO), specify the endpoint URL." +
    " Leave empty for standard AWS S3.";


/**
 * Renders S3-specific form items for compression job submission.
 *
 * @return
 */
const S3InputFormItems = () => {
    const bucket = Form.useWatch<string>("bucket");
    const regionCode = Form.useWatch<string>("regionCode");
    const endpointUrl = Form.useWatch<string>("endpointUrl");

    return (
        <>
            <Form.Item
                label={"S3 Bucket"}
                name={"bucket"}
                rules={[{required: true, message: "Please enter an S3 bucket name"}]}
                tooltip={BUCKET_HELPER_TEXT}
            >
                <Input placeholder={BUCKET_PLACEHOLDER_TEXT}/>
            </Form.Item>
            <Form.Item
                label={"Region"}
                name={"regionCode"}
                tooltip={REGION_CODE_HELPER_TEXT}
            >
                <Input placeholder={REGION_CODE_PLACEHOLDER_TEXT}/>
            </Form.Item>
            <Form.Item
                label={"Endpoint URL"}
                name={"endpointUrl"}
                tooltip={ENDPOINT_URL_HELPER_TEXT}
            >
                <Input placeholder={ENDPOINT_URL_PLACEHOLDER_TEXT}/>
            </Form.Item>
            <S3KeySelectFormItem
                bucket={bucket}
                endpointUrl={endpointUrl}
                regionCode={regionCode}/>
        </>
    );
};


export default S3InputFormItems;
