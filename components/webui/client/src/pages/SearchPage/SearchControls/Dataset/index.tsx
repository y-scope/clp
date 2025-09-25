import InputLabel from "../../../../components/InputLabel";
import DatasetSelect from "./DatasetSelect";
import styles from "./index.module.css";


/**
 * Renders the dataset selector component with an input label.
 *
 * @return
 */
const Dataset = () => {
    return (
        <div className={styles["datasetContainer"]}>
            <InputLabel>Dataset</InputLabel>
            <DatasetSelect className={styles["select"] || ""}/>
        </div>
    );
};

export default Dataset;
