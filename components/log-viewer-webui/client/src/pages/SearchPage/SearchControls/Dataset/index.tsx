import {Select, theme, Typography, message} from "antd";
import {useQuery} from "@tanstack/react-query";
import {useEffect} from "react";
import useSearchStore from "../../SearchState/index";
import useIngestStatsStore from "../../../IngestPage/ingestStatsStore";
import styles from "./index.module.css";
import {fetchDatasetsRaw} from "./datasetSql";

const {Option} = Select;
const {Text} = Typography;

// Plan is the two states.
// UI states starts as null, then there is a hook that updates it. Maybe is Success. Use same trick with null. Like if not null
// In UI, should disabled submit button when no data. Should also disable timeline zoom. Tooltip should change to say no datasets. State must also be clp-s
// If there is an error, should display a message. Error fetching data
// If there is no error, but no data should display something as well.  maybe can do warning. Maybe put message. saying please ingest data.
// Note this whole component should not render

const Dataset = () => {
    const {token} = theme.useToken();
    const {refreshInterval} = useIngestStatsStore();

    const dataset = useSearchStore((state) => state.selectDataset);
    const updateDataset = useSearchStore((state) => state.updateSelectDataset);

    const [messageApi, contextHolder] = message.useMessage();

    const {data, isPending, isSuccess, error} = useQuery({
        queryKey: ['datasets'],
        queryFn: fetchDatasetsRaw,
        staleTime: refreshInterval,
        initialData: [],
    });

    // Set first dataset as default when data loads and no dataset is selected
    useEffect(() => {
        if (isSuccess) {
            if ("undefined" !== typeof data[0] && null === dataset) {
                updateDataset(data[0]);
            }
        }
    }, [isSuccess, data, dataset, updateDataset]);

    // Handle error messages - they auto-disappear so no cleanup needed
    useEffect(() => {
        if (error) {
            messageApi.error({
                key: 'fetchError',
                content: 'Error fetching datasets.',
            });
        }
    }, [error]);

        // Handle error messages - they auto-disappear so no cleanup needed
    useEffect(() => {
        if (isSuccess && data.length === 0) {
            messageApi.warning({
                key: 'noData',
                content: 'There is no data ingested yet. Please ingest data to begin search',
                duration: 0,
            });
        }
    }, [data, isSuccess]);

    const handleDatasetChange = (value: string) => {
        updateDataset(value);
    };

    let testdata = ["hello, world", "test dataset", "another dataset"];

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
                options={testdata.map((option) => ({label: option, value: option}))}
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
