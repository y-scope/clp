import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import guidedGrid from "./index.module.css";


/**
 * Renders the WHERE SQL input field.
 *
 * @return
 */
const Where = () => (
    <div className={guidedGrid["where"]}>
        <InputLabel> WHERE </InputLabel>
        <SqlInput
            className={guidedGrid[`noLeftBorderRadius`] || ""}
            disabled={false}/>
    </div>
);

export default Where;
