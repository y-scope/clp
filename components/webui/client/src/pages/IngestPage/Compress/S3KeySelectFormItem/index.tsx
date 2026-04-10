import {
    useCallback,
    useEffect,
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
 * Form item with TreeSelect for browsing and selecting S3 keys/prefixes.
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
            name={"s3Paths"}
            rules={[{required: true, message: "Please select at least one S3 key or prefix"}]}
            label={isScanner ?
                "S3 Prefixes" :
                "S3 Keys"}
            tooltip={"Select S3 object keys or prefixes to compress." +
                " Selecting a prefix compresses all objects under it."}
            validateStatus={s3Error ?
                "error" :
                ""}
        >
            <TreeSelect
                allowClear={true}
                listHeight={listHeight}
                multiple={true}
                notFoundContent={null}
                showCheckedStrategy={TreeSelect.SHOW_PARENT}
                showSearch={false}
                switcherIcon={SwitcherIcon}
                treeCheckable={true}
                treeDataSimpleMode={true}
                treeExpandedKeys={expandedKeys}
                treeLine={true}
                treeNodeLabelProp={"value"}
                {...(hasBucketAndRegion && {loadData: handleLoadData})}
                placeholder={hasBucketAndRegion ?
                    "Browse and select S3 keys or prefixes" :
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
