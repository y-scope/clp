import {useMutation} from "@tanstack/react-query";
import {
    type S3ScannerCreation,
    validateS3ScannerInput,
} from "@webui/common/schemas/ingestion";
import {
    Alert,
    Button,
    Form,
    Input,
    Typography,
} from "antd";

import {submitS3Scanner} from "../../../api/ingest";
import {DashboardCard} from "../../../components/DashboardCard";
import ClpSFormItems from "../Compress/ClpSFormItems";


const s3Inputs = [
    {label: "S3 Bucket", name: "bucket", placeholder: "my-log-bucket"},
    {label: "AWS Region", name: "region", placeholder: "us-east-2"},
    {label: "Key Prefix", name: "keyPrefix", placeholder: "logs/"},
] as const;

/**
 * Submits one continuously running S3 scanner using deployment-owned authentication.
 *
 * @return
 */
const S3Scanner = () => {
    const [form] = Form.useForm<S3ScannerCreation>();
    const {mutate, isPending, isSuccess, data, error} = useMutation({
        mutationFn: submitS3Scanner,
        retry: false,
    });

    return (
        <DashboardCard title={"Create S3 Scanner"}>
            <Typography.Paragraph>
                Continuously ingest logs from an S3 bucket and prefix.
            </Typography.Paragraph>
            <Form
                disabled={isPending}
                form={form}
                layout={"vertical"}
                onFinish={(values) => {
                    if (!isPending) {
                        mutate(values);
                    }
                }}
            >
                {s3Inputs.map(({label, name, placeholder}) => (
                    <Form.Item
                        key={name}
                        label={label}
                        name={name}
                        required={true}
                        rules={[{
                            validator: (_, value: unknown) => {
                                const message = validateS3ScannerInput(name, value);

                                return message ?
                                    Promise.reject(new Error(message)) :
                                    Promise.resolve();
                            },
                        }]}
                    >
                        <Input placeholder={placeholder}/>
                    </Form.Item>
                ))}
                <ClpSFormItems/>
                <Form.Item>
                    <Button
                        disabled={isPending}
                        htmlType={"submit"}
                        loading={isPending}
                        type={"primary"}
                    >
                        Create Scanner
                    </Button>
                </Form.Item>
            </Form>
            {isSuccess && (
                <Alert
                    showIcon={true}
                    title={`Scanner created with ID: ${String(data.id)}`}
                    type={"success"}
                    description={"Creation does not confirm successful ingestion. Scanner jobs " +
                        "are not yet displayed in the Jobs table."}/>
            )}
            {error && <Alert
                showIcon={true}
                title={error.message}
                type={"error"}/>}
        </DashboardCard>
    );
};


export default S3Scanner;
