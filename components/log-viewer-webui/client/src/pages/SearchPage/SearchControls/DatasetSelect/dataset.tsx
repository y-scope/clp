import {Select, theme, Typography} from "antd";
import {useQuery} from "@tanstack/react-query";
import useSearchStore from "../../SearchState/index";
import styles from "./dataset.module.css";
import {fetchDatasetsRaw} from "./datasetSql";
import {useEffect} from "react";

const {Option} = Select;
const {useToken} = theme;
const {Text} = Typography;

const DatasetSelect = () => {
    const dataset = useSearchStore((state) => state.selectDataset);
    const updateDataset = useSearchStore((state) => state.updateSelectDataset);
    const {token} = useToken();

    const {data = [], isLoading, error} = useQuery<string[], Error>({
        queryKey: ['datasets'],
        queryFn: fetchDatasetsRaw,
        staleTime: 5 * 60 * 1000, // 5 minutes
    });

    // Set first dataset as default when data loads and no dataset is selected
    useEffect(() => {
        if (data.length > 0 && !dataset) {
            updateDataset(data[0]);
        }
    }, [data, dataset, updateDataset]);

    const handleDatasetChange = (value: string) => {
        updateDataset(value);
        console.log('Selected dataset:', value);
    };

    if (error) {

        console.error("Failed to fetch datasets", error);

    }

    return (
        <div className={styles['datasetContainer']}>
            <Text
                className={styles['labelBox'] || ""}
                style={{
                    backgroundColor: token.colorFillAlter,
                    borderColor: token.colorBorder,
                    borderTopLeftRadius: `${token.borderRadius}px`,
                    borderBottomLeftRadius: `${token.borderRadius}px`,
                    borderTopRightRadius: 0,
                    borderBottomRightRadius: 0,
                    fontSize: token.fontSizeLG
                }}
            >
                Dataset
            </Text>
            <Select
                className={styles['datasetSelect'] || ""}
                placeholder="Select a dataset"
                showSearch
                size={"large"}
                style={{ width: 200, minWidth: 200 }}
                onChange={handleDatasetChange}
                loading={isLoading}
                value={dataset}
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

export default DatasetSelect;
