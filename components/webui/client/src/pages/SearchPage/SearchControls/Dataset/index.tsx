import InputLabel from "../../../../components/InputLabel";
import useSearchStore from "../../SearchState/index";
import DatasetSelect from "./DatasetSelect";
import styles from "./index.module.css";


/**
 * Renders a dataset selector with an input label.
 *
 * @return
 */
const Dataset = () => {
    const datasets = useSearchStore((state) => state.selectedDatasets);
    const expanded = 1 < datasets.length;

    return (
        <div className={styles["datasetContainer"]}>
            <InputLabel>Dataset</InputLabel>
            <div
                className={`${styles["selectContainer"]}${expanded ?
                    ` ${styles["selectContainerExpanded"]}` :
                    ""}`}
            >
                <DatasetSelect
                    className={styles["select"] || ""}
                    {...(expanded ?
                        {maxTagCount: "responsive"} :
                        {})}/>
            </div>
        </div>
    );
};

export default Dataset;
