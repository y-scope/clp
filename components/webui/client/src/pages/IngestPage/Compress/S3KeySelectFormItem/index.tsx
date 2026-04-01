import {
    useCallback,
    useEffect,
    useState,
} from "react";

import {
    Form,
    message,
    TreeSelect,
    TreeSelectProps,
} from "antd";

import SwitcherIcon from "../PathsSelectFormItem/SwitcherIcon";
import useS3Tree, {
    LOAD_MORE_PREFIX,
    S3ConnectionConfig,
} from "./useS3Tree";
import {getListHeight} from "./utils";


type LoadDataNode = Parameters<NonNullable<TreeSelectProps["loadData"]>>[0];

type TreeExpandKeys = Parameters<NonNullable<TreeSelectProps["onTreeExpand"]>>[0];

/**
 * Form item with TreeSelect for browsing and selecting S3 keys/prefixes.
 *
 * @param props
 * @param props.bucket
 * @param props.endpointUrl
 * @param props.regionCode
 * @return
 */
const S3KeySelectFormItem = ({bucket, endpointUrl, regionCode}: S3ConnectionConfig) => {
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
    } = useS3Tree({bucket, endpointUrl, regionCode});

    const handleLoadData = useCallback(async ({value}: LoadDataNode) => {
        if ("string" !== typeof value) {
            return;
        }
        if (value.startsWith(LOAD_MORE_PREFIX)) {
            return;
        }
        try {
            await loadPrefix(value);
        } catch (e) {
            console.error("Failed to load S3 prefix:", e);
            message.error(e instanceof Error ?
                e.message :
                "Unknown error while listing S3 objects");
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
    }, [
        treeData,
        handleLoadMore,
    ]);

    const isDisabled = !bucket;

    return (
        <Form.Item
            label={"S3 Keys"}
            name={"s3Paths"}
            rules={[{required: true, message: "Please select at least one S3 key or prefix"}]}
            tooltip={"Select S3 object keys or prefixes to compress." +
                " Selecting a prefix compresses all objects under it."}
        >
            <TreeSelect
                allowClear={true}
                disabled={isDisabled}
                listHeight={listHeight}
                loadData={handleLoadData}
                multiple={true}
                showCheckedStrategy={TreeSelect.SHOW_PARENT}
                showSearch={false}
                switcherIcon={SwitcherIcon}
                treeCheckable={true}
                treeData={treeData}
                treeDataSimpleMode={true}
                treeExpandedKeys={expandedKeys}
                treeLine={true}
                treeNodeLabelProp={"value"}
                placeholder={isDisabled ?
                    "Enter a bucket name first" :
                    "Browse and select S3 keys or prefixes"}
                onSelect={handleSelect}
                onTreeExpand={handleTreeExpand}/>
        </Form.Item>
    );
};


export default S3KeySelectFormItem;
