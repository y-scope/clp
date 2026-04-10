import {useState} from "react";

import {
    AutoComplete,
    Col,
    Form,
    Input,
    Row,
} from "antd";

import {AWS_REGION_CODES} from "./awsRegionCodes";
import S3KeySelectFormItem from "./S3KeySelectFormItem";


const BUCKET_PLACEHOLDER_TEXT = "my-logs-bucket";

const BUCKET_TOOLTIP_TEXT = "The S3 bucket containing the logs to compress.";

const REGION_PLACEHOLDER_TEXT = "us-east-1";

const REGION_TOOLTIP_TEXT = "The region where the bucket is located.";


/**
 * Region options for the AutoComplete, derived from the full AWS region list.
 */
const REGION_OPTIONS = AWS_REGION_CODES.map((code) => ({value: code}));

/**
 * Renders S3-specific form items for compression job submission.
 *
 * @param props
 * @param props.isScanner
 * @return
 */
const S3InputFormItems = ({isScanner = false}: {isScanner?: boolean}) => {
    const bucket = Form.useWatch<string>("bucket");
    const regionCode = Form.useWatch<string>("regionCode");

    const [confirmedBucket, setConfirmedBucket] = useState<string>();
    const [confirmedRegionCode, setConfirmedRegionCode] = useState<string>();
    const [s3Error, setS3Error] = useState<string | null>(null);

    return (
        <>
            <Row gutter={8}>
                <Col span={8}>
                    <Form.Item
                        label={"Region"}
                        name={"regionCode"}
                        rules={[{required: true, message: "Please enter a region"}]}
                        tooltip={REGION_TOOLTIP_TEXT}
                    >
                        <AutoComplete
                            filterOption={true}
                            options={REGION_OPTIONS}
                            placeholder={REGION_PLACEHOLDER_TEXT}
                            onBlur={() => {
                                setConfirmedRegionCode(regionCode);
                            }}
                            onSelect={(value: string) => {
                                setConfirmedRegionCode(value);
                            }}/>
                    </Form.Item>
                </Col>
                <Col span={16}>
                    <Form.Item
                        label={"S3 Bucket"}
                        name={"bucket"}
                        rules={[{required: true, message: "Please enter a bucket name"}]}
                        tooltip={BUCKET_TOOLTIP_TEXT}
                    >
                        <Input
                            placeholder={BUCKET_PLACEHOLDER_TEXT}
                            onBlur={() => {
                                setConfirmedBucket(bucket);
                            }}/>
                    </Form.Item>
                </Col>
            </Row>
            <S3KeySelectFormItem
                bucket={confirmedBucket}
                isScanner={isScanner}
                regionCode={confirmedRegionCode}
                s3Error={s3Error}
                onError={setS3Error}/>
        </>
    );
};


export default S3InputFormItems;
