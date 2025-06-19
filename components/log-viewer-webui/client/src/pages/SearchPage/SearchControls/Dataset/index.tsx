import {Select, theme, Typography, message} from "antd";
import {useQuery} from "@tanstack/react-query";
import {useEffect} from "react";
import useSearchStore from "../../SearchState/index";
import useIngestStatsStore from "../../../IngestPage/ingestStatsStore";
import styles from "./index.module.css";
import {fetchDatasetNames} from "./sql";

const {Text} = Typography;

const Dataset = () => {
    const {token} = theme.useToken();
    const {refreshInterval} = useIngestStatsStore();

    const dataset = useSearchStore((state) => state.selectDataset);
    const updateDataset = useSearchStore((state) => state.updateSelectDataset);

    const [messageApi, contextHolder] = message.useMessage();

    const {data, isPending, isSuccess, error} = useQuery({
        queryKey: ['datasets'],
        queryFn: fetchDatasetNames,
        staleTime: refreshInterval,
        initialData: [],
    });

    // Update the selected dataset to the first dataset in the the reponse. The dataset is only
    // updated if the dataset is not already set (i.e. on initial response).
    useEffect(() => {
        if (isSuccess) {
            if ("undefined" !== typeof data[0] && null === dataset) {
                updateDataset(data[0]);
            }
        }
    }, [isSuccess, data, dataset, updateDataset]);

    // Display error message if the query fails since querying is disabled if no datasets.
    useEffect(() => {
        if (error) {
            messageApi.error({
                key: 'fetchError',
                content: 'Error fetching datasets.',
            });
        }
    }, [error]);

    // Display warning message if there are no datasets since querying is disabled if no datasets.
    useEffect(() => {
        if (isSuccess && data.length === 0) {
            messageApi.warning({
                key: 'noData',
                content: 'There is no data ingested yet. Please ingest data to search.',
                duration: 0,
            });
        }
    }, [data, isSuccess]);

    const handleDatasetChange = (value: string) => {
        updateDataset(value);
    };

    return (
        <div className={styles['datasetContainer']}>
            {contextHolder}
            <Text
                className={styles['labelBox'] || ""}
                style={{
                    backgroundColor: token.colorFillAlter,
                    borderColor: token.colorBorder,
                    borderTopLeftRadius: `${token.borderRadius}px`,
                    borderBottomLeftRadius: `${token.borderRadius}px`,
                    fontSize: token.fontSizeLG
                }}
            >
                Dataset
            </Text>
            <Select
                className={styles['select'] || ""}
                placeholder="Load..."
                size={"large"}
                onChange={handleDatasetChange}
                loading={isPending}
                value={dataset}
                showSearch
                options={data.map((option) => ({label: option, value: option}))}
                optionFilterProp="label"
                filterSort={(optionA, optionB) =>
                    (optionA?.label ?? '').toLowerCase().localeCompare((optionB?.label ?? '').toLowerCase())
                }
            >
            </Select>
        </div>
    );
};

export default Dataset;
