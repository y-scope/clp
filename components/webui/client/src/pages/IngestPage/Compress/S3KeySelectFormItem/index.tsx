import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import {
    Form,
    TreeSelect,
    TreeSelectProps,
} from "antd";

import SwitcherIcon from "../PathsSelectFormItem/SwitcherIcon";
import useS3Tree, {
    getErrorMessage,
    LOAD_MORE_PREFIX,
} from "./useS3Tree";
import {getListHeight} from "./utils";


type LoadDataNode = Parameters<NonNullable<TreeSelectProps["loadData"]>>[0];

type TreeExpandKeys = Parameters<NonNullable<TreeSelectProps["onTreeExpand"]>>[0];

/**
 * Form item with TreeSelect for browsing and selecting an S3 key/prefix.
 *
 * @param props
 * @param props.bucket
 * @param props.onError
 * @param props.regionCode
 * @param props.s3Error
 * @param props.isScanner
 * @return
 */
const S3KeySelectFormItem = ({
    bucket,
    isScanner = false,
    onError,
    regionCode,
    s3Error,
}: {
    bucket: string | undefined;
    isScanner?: boolean;
    onError?: (msg: string | null) => void;
    regionCode: string | undefined;
    s3Error: string | null;
}) => {
    const [listHeight, setListHeight] = useState<number>(getListHeight);
    const lastValidValue = useRef<string | null>(null);

    useEffect(() => {
        const handleResize = () => {
            setListHeight(getListHeight());
        };

        window.addEventListener("resize", handleResize);

        return () => {
            window.removeEventListener("resize", handleResize);
        };
    }, []);

    const {
        expandedKeys,
        handleLoadMore,
        loadPrefix,
        setExpandedKeys,
        treeData,
    } = useS3Tree(bucket, regionCode, onError);

    const handleLoadData = useCallback(async ({value}: LoadDataNode) => {
        if ("string" !== typeof value) {
            return;
        }
        const strValue: string = value;
        if (strValue.startsWith(LOAD_MORE_PREFIX)) {
            return;
        }
        try {
            await loadPrefix(strValue);
        } catch (e) {
            console.error("Failed to load S3 prefix:", e);
            onError?.(getErrorMessage(e, "S3 bucket or region not found."));
        }
    }, [loadPrefix,
        onError]);

    const handleTreeExpand = useCallback((keys: TreeExpandKeys) => {
        setExpandedKeys(keys as string[]);
    }, [setExpandedKeys]);

    const handleSelect = useCallback((value: string) => {
        const node = treeData.find((n) => n.id === value);
        if (node?.isLoadMore) {
            handleLoadMore(node);
        }
    }, [
        treeData,
        handleLoadMore,
    ]);

    const hasBucketAndRegion = bucket && regionCode;

    return (
        <Form.Item
            help={s3Error || ""}
            name={"s3Path"}
            rules={[{required: true, message: "Please select an S3 key or prefix"}]}
            getValueFromEvent={(value: string) => {
                // Filter out load-more pseudo-values so they never become the
                // form field value; restore the last valid selection instead.
                if ("string" === typeof value && value.startsWith(LOAD_MORE_PREFIX)) {
                    return lastValidValue.current;
                }
                lastValidValue.current = value;

                return value;
            }}
            label={isScanner ?
                "S3 Prefix" :
                "S3 Key"}
            tooltip={"Select an S3 object key or prefix to compress." +
                " Selecting a prefix compresses all objects under it."}
            validateStatus={s3Error ?
                "error" :
                ""}
        >
            <TreeSelect
                allowClear={true}
                listHeight={listHeight}
                notFoundContent={null}
                showSearch={false}
                switcherIcon={SwitcherIcon}
                treeDataSimpleMode={true}
                treeExpandedKeys={expandedKeys}
                treeLine={true}
                treeNodeLabelProp={"value"}
                {...(hasBucketAndRegion && {loadData: handleLoadData})}
                placeholder={hasBucketAndRegion ?
                    "Browse and select an S3 key or prefix" :
                    "Enter a bucket name and region first"}
                treeData={null !== s3Error ?
                    [] :
                    treeData}
                onSelect={handleSelect}
                onTreeExpand={handleTreeExpand}/>
        </Form.Item>
    );
};


export default S3KeySelectFormItem;
