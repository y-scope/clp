import InputLabel from "../../../../../components/InputLabel";
import DatasetSelect from "../../Dataset/DatasetSelect";
import guidedGrid from "./index.module.css";


/**
 * Renders the FROM SQL input field.
 *
 * @return
 */
const From = () => {
    return (
        <div className={guidedGrid["from"]}>
            <InputLabel>FROM</InputLabel>
            <DatasetSelect
                className={
                    `${guidedGrid["noLeftBorderRadiusSelect"]} ${
                        guidedGrid["widthSelect"]}`
                }/>
        </div>
    );
};

export default From;
