import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import {
    MinusOutlined,
    PlusOutlined,
} from "@ant-design/icons";
import {
    Button,
    Form,
    TreeSelect,
    TreeSelectProps,
} from "antd";

import SwitcherIcon from "./PathsSelectFormItem/SwitcherIcon";
import useS3Tree, {
    LOAD_MORE_PREFIX,
    getErrorMessage,
} from "./S3KeySelectFormItem/useS3Tree";
import {getListHeight} from "./S3KeySelectFormItem/utils";


type LoadDataNode = Parameters<NonNullable<TreeSelectProps["loadData"]>>[0];
type TreeExpandKeys = Parameters<NonNullable<TreeSelectProps["onTreeExpand"]>>[0];

/**
 * Form.List of single-select TreeSelect rows for S3 key/prefix selection,
 * with +/- buttons to add/remove rows. Shares a single S3 tree state.
 *
 * @param props
 * @param props.bucket
 * @param props.isScanner
 * @param props.regionCode
 * @return
 */
const S3KeysFormItem = ({
    bucket,
    isScanner = false,
    regionCode,
}: {
    bucket: string | undefined;
    isScanner?: boolean;
    regionCode: string | undefined;
}) => {
    const [s3Error, setS3Error] = useState<string | null>(null);
    const [listHeight, setListHeight] = useState<number>(getListHeight);
    const lastValidValues = useRef(new Map<number, string>());

    const {
        expandedKeys,
        handleLoadMore,
        loadPrefix,
        setExpandedKeys,
        treeData,
    } = useS3Tree(bucket, regionCode, (msg) => {
        setS3Error(msg);
    });

    const handleLoadData = useCallback(async ({value}: LoadDataNode) => {
        if ("string" !== typeof value || value.startsWith(LOAD_MORE_PREFIX)) {
            return;
        }
        try {
            await loadPrefix(value);
        } catch (e) {
            console.error("Failed to load S3 prefix:", e);
            setS3Error(getErrorMessage(e, "S3 bucket or region not found."));
        }
    }, [loadPrefix]);

    const handleTreeExpand = useCallback((keys: TreeExpandKeys) => {
        setExpandedKeys(keys as string[]);
    }, [setExpandedKeys]);

    const handleSelect = useCallback((value: string) => {
        const node = treeData.find((n) => n.id === value);
        if (node?.isLoadMore) {
            handleLoadMore(node);
        }
    }, [treeData, handleLoadMore]);

    const hasBucketAndRegion = bucket && regionCode;
    const isDisabled = !hasBucketAndRegion || null !== s3Error;

    // Clear stale selections when the tree becomes disabled (bucket/region
    // changed to invalid or S3 error occurred), so the placeholder shows.
    const form = Form.useFormInstance();
    useEffect(() => {
        if (isDisabled) {
            form.setFieldValue("s3Paths", [null]);
        }
    }, [isDisabled, form]);

    useEffect(() => {
        const handleResize = () => {
            setListHeight(getListHeight());
        };

        window.addEventListener("resize", handleResize);

        return () => {
            window.removeEventListener("resize", handleResize);
        };
    }, []);

    return (
        <>
            <Form.Item
                help={s3Error || ""}
                label={isScanner ? "S3 Prefixes" : "S3 Keys"}
                required={true}
                tooltip={"Select S3 object keys or prefixes to compress." +
                    " Selecting a prefix compresses all objects under it."}
                validateStatus={s3Error ? "error" : ""}
            >
                <Form.List
                    initialValue={[null]}
                    name={"s3Paths"}
                >
                    {(fields, {add, remove}) => (
                        <>
                            {fields.map((field, index) => (
                                <div
                                    key={field.key}
                                    style={{display: "flex", gap: 8, marginBottom: 8}}
                                >
                                    <Form.Item
                                        getValueFromEvent={(value: string) => {
                                            if ("string" === typeof value &&
                                                value.startsWith(LOAD_MORE_PREFIX)
                                            ) {
                                                return lastValidValues.current.get(field.name) ?? null;
                                            }
                                            lastValidValues.current.set(field.name, value);

                                            return value;
                                        }}
                                        name={field.name}
                                        noStyle
                                        {...(0 === index ? {
                                            rules: [{
                                                // Accept "" (bucket root) as a valid selection
                                                validator: async (_, val) => {
                                                    if (null === val || undefined === val) {
                                                        throw new Error("Please select an S3 key or prefix");
                                                    }
                                                },
                                            }],
                                        } : {})}
                                    >
                                        <TreeSelect
                                            allowClear={false}
                                            disabled={isDisabled}
                                            listHeight={listHeight}
                                            showSearch={false}
                                            style={{width: "100%"}}
                                            switcherIcon={SwitcherIcon}
                                            treeDataSimpleMode={true}
                                            treeExpandedKeys={expandedKeys}
                                            treeLine={true}
                                            treeNodeLabelProp={"label"}
                                            {...(hasBucketAndRegion && {loadData: handleLoadData})}
                                            placeholder={isDisabled ?
                                                "Enter a valid region and bucket" :
                                                "Browse and select an S3 key or prefix"}
                                            treeData={null !== s3Error ?
                                                [] :
                                                treeData}
                                            onSelect={handleSelect}
                                            onTreeExpand={handleTreeExpand}
                                        />
                                    </Form.Item>
                                    {1 < fields.length && (
                                        <Button
                                            disabled={isDisabled}
                                            icon={<MinusOutlined/>}
                                            onClick={() => {
                                                remove(field.name);
                                            }}
                                        />
                                    )}
                                    {index === fields.length - 1 && (
                                        <Button
                                            disabled={isDisabled}
                                            icon={<PlusOutlined/>}
                                            onClick={() => {
                                                add(null);
                                            }}
                                        />
                                    )}
                                </div>
                            ))}
                        </>
                    )}
                </Form.List>
            </Form.Item>
        </>
    );
};


export default S3KeysFormItem;