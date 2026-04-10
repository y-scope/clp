import {
    Collapse,
    Form,
    InputNumber,
} from "antd";


const SCANNING_INTERVAL_DEFAULT_SEC = 30;

// 4 GiB
const BUFFER_FLUSH_THRESHOLD_DEFAULT_BYTES = 4_294_967_296;
const BUFFER_TIMEOUT_DEFAULT_SEC = 300;
const BUFFER_CHANNEL_CAPACITY_DEFAULT = 16;


/**
 * Renders the "Advanced" collapse panel for scanner ingestion jobs.
 *
 * @return
 */
const ScannerAdvancedFormItems = () => {
    return (
        <Collapse
            items={[{
                children: (
                    <>
                        <Form.Item
                            initialValue={SCANNING_INTERVAL_DEFAULT_SEC}
                            label={"Scanning Interval (seconds)"}
                            name={"scanningIntervalSec"}
                            tooltip={"How often the scanner polls S3 for new objects."}
                        >
                            <InputNumber
                                min={1}
                                style={{width: "100%"}}/>
                        </Form.Item>
                        <Form.Item
                            initialValue={BUFFER_FLUSH_THRESHOLD_DEFAULT_BYTES}
                            label={"Buffer Flush Threshold (bytes)"}
                            name={"bufferFlushThresholdBytes"}
                            tooltip={
                                "Flush the buffer once total buffered object size exceeds" +
                                " this threshold."
                            }
                        >
                            <InputNumber
                                min={1}
                                style={{width: "100%"}}/>
                        </Form.Item>
                        <Form.Item
                            initialValue={BUFFER_TIMEOUT_DEFAULT_SEC}
                            label={"Buffer Timeout (seconds)"}
                            name={"bufferTimeoutSec"}
                            tooltip={
                                "Hard timeout: flush the buffer when the oldest buffered" +
                                " object has waited at least this long."
                            }
                        >
                            <InputNumber
                                min={1}
                                style={{width: "100%"}}/>
                        </Form.Item>
                        <Form.Item
                            initialValue={BUFFER_CHANNEL_CAPACITY_DEFAULT}
                            label={"Buffer Channel Capacity"}
                            name={"bufferChannelCapacity"}
                            tooltip={
                                "Maximum number of objects queued before being processed by" +
                                " the buffer."
                            }
                        >
                            <InputNumber
                                min={1}
                                style={{width: "100%"}}/>
                        </Form.Item>
                    </>
                ),
                key: "advanced",
                label: "Advanced",
            }]}/>
    );
};


export default ScannerAdvancedFormItems;
