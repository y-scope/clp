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
    getErrorMessage,
    LOAD_MORE_PREFIX,
} from "./S3KeySelectFormItem/useS3Tree";
import {getListHeight} from "./S3KeySelectFormItem/utils";


type LoadDataNode = Parameters<NonNullable<TreeSelectProps["loadData"]>>[0];

type TreeExpandKeys = Parameters<NonNullable<TreeSelectProps["onTreeExpand"]>>[0];

/**
 * Validates that the first S3 key/prefix row has a non-null selection.
 * Accepts "" (empty string = bucket root) as valid.
 *
 * @param _
 * @param val
 * @return
 */
const validateFirstRow = (_: unknown, val: string | null) => {
    if (null === val || "undefined" === typeof val) {
        return Promise.reject(new Error(
            "Please select an S3 key or prefix",
        ));
    }

    return Promise.resolve();
};

const FIRST_ROW_RULES = [{validator: validateFirstRow}];

/**
 * Props for the S3 TreeSelect row sub-component.
 */
type S3TreeSelectRowProps = {
    expandedKeys: string[];
    field: {key: React.Key; name: number};
    fieldIndex: number;
    fieldsLength: number;
    hasBucketAndRegion: boolean;
    isDisabled: boolean;
    lastValidValues: React.RefObject<Map<number, string>>;
    listHeight: number;
    onAdd: () => void;
    onLoadData: (node: LoadDataNode) => Promise<void>;
    onRemove: () => void;
    onSelect: (value: string) => void;
    onTreeExpand: (keys: TreeExpandKeys) => void;
    s3Error: string | null;
    treeData: S3TreeNode[];
};

type S3TreeNode = import("./S3KeySelectFormItem/typings").S3TreeNode;

/**
 * Renders a single TreeSelect row with +/- action buttons.
 *
 * @param props
 * @param props.expandedKeys
 * @param props.field
 * @param props.fieldIndex
 * @param props.fieldsLength
 * @param props.hasBucketAndRegion
 * @param props.isDisabled
 * @param props.lastValidValues
 * @param props.listHeight
 * @param props.onAdd
 * @param props.onLoadData
 * @param props.onRemove
 * @param props.onSelect
 * @param props.onTreeExpand
 * @param props.s3Error
 * @param props.treeData
 * @return
 */
const S3TreeSelectRow = ({
    expandedKeys,
    field,
    fieldIndex,
    fieldsLength,
    hasBucketAndRegion,
    isDisabled,
    lastValidValues,
    listHeight,
    onAdd,
    onLoadData,
    onRemove,
    onSelect,
    onTreeExpand,
    s3Error,
    treeData,
}: S3TreeSelectRowProps) => (
    <div
        key={field.key}
        style={{display: "flex", gap: 8, marginBottom: 8}}
    >
        <Form.Item
            name={field.name}
            noStyle={true}
            getValueFromEvent={(value: string) => {
                if ("string" === typeof value &&
                    value.startsWith(LOAD_MORE_PREFIX)
                ) {
                    return lastValidValues.current.get(field.name) ?? null;
                }
                lastValidValues.current.set(field.name, value);

                return value;
            }}
            rules={0 === fieldIndex ?
                FIRST_ROW_RULES :
                []}
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
                {...(hasBucketAndRegion && {loadData: onLoadData})}
                placeholder={isDisabled ?
                    "Enter a valid region and bucket" :
                    "Browse and select an S3 key or prefix"}
                treeData={null !== s3Error ?
                    [] :
                    treeData}
                onSelect={onSelect}
                onTreeExpand={onTreeExpand}/>
        </Form.Item>
        {1 < fieldsLength && (
            <Button
                disabled={isDisabled}
                icon={<MinusOutlined/>}
                onClick={onRemove}/>
        )}
        {fieldIndex === fieldsLength - 1 && (
            <Button
                disabled={isDisabled}
                icon={<PlusOutlined/>}
                onClick={onAdd}/>
        )}
    </div>
);

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
    }, [treeData,
        handleLoadMore]);

    const hasBucketAndRegion = Boolean(bucket && regionCode);
    const isDisabled = !hasBucketAndRegion || null !== s3Error;

    // Clear stale selections when the tree becomes disabled (bucket/region
    // changed to invalid or S3 error occurred), so the placeholder shows.
    const form = Form.useFormInstance();
    useEffect(() => {
        if (isDisabled) {
            form.setFieldValue("s3Paths", [null]);
        }
    }, [isDisabled,
        form]);

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
        <Form.Item
            help={s3Error || ""}
            required={true}
            label={isScanner ?
                "S3 Prefixes" :
                "S3 Keys"}
            tooltip={"Select S3 object keys or prefixes to compress." +
                " Selecting a prefix compresses all objects under it."}
            validateStatus={s3Error ?
                "error" :
                ""}
        >
            <Form.List
                initialValue={[null]}
                name={"s3Paths"}
            >
                {(fields, {add, remove}) => (
                    <>
                        {fields.map((field, index) => (
                            <S3TreeSelectRow
                                expandedKeys={expandedKeys}
                                field={field}
                                fieldIndex={index}
                                fieldsLength={fields.length}
                                hasBucketAndRegion={hasBucketAndRegion}
                                isDisabled={isDisabled}
                                key={field.key}
                                lastValidValues={lastValidValues}
                                listHeight={listHeight}
                                s3Error={s3Error}
                                treeData={treeData}
                                onLoadData={handleLoadData}
                                onSelect={handleSelect}
                                onTreeExpand={handleTreeExpand}
                                onAdd={() => {
                                    add(null);
                                }}
                                onRemove={() => {
                                    remove(field.name);
                                }}/>
                        ))}
                    </>
                )}
            </Form.List>
        </Form.Item>
    );
};

export default S3KeysFormItem;
