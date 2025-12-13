import InputLabel from "../../../../../../components/InputLabel";
import DatasetSelect from "../../../Dataset/DatasetSelect";
import guidedGrid from "./index.module.css";
import {LABEL_WIDTH} from "./typings";


/**
 * Renders the FROM SQL input field.
 *
 * @return
 */
const From = () => {
    return (
        <div className={guidedGrid["gridItem"]}>
            <InputLabel width={LABEL_WIDTH}>FROM</InputLabel>
            <DatasetSelect
                className={
                    `${guidedGrid["noLeftBorderRadiusSelect"]} ${
                        guidedGrid["widthSelect"]}`
                }/>
        </div>
    );
};

export default From;
