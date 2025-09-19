import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import guidedGrid from "./index.module.css";


/**
 * Renders the SELECT SQL input field.
 *
 * @return
 */
const Select = () => (
    <div className={guidedGrid["select"]}>
        <InputLabel>SELECT</InputLabel>
        <SqlInput
            className={guidedGrid["noLeftBorderRadius"] || ""}
            disabled={false}/>
    </div>
);

export default Select;
