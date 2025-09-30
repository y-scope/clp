import InputLabel from "../../../../components/InputLabel";
import styles from "./index.module.css";
import DatasetSelect from "./DatasetSelect";


/**
 * Renders a dataset selector component that allows users to select from available datasets.
 *
 * @return
 */
const Dataset = () => {
    return (
        <div className={styles["datasetContainer"]}>
            <InputLabel>Dataset</InputLabel>
            <DatasetSelect className={styles["select"] || ""} />
        </div>
    );
};

export default Dataset;
