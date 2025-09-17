import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import guidedGrid from "./index.module.css";


/**
 * Renders the ORDER BY SQL input field.
 *
 * @return
 */
const OrderBy = () => (
    <div className={guidedGrid["order"]}>
        <InputLabel> ORDER BY </InputLabel>
        <SqlInput
            className={guidedGrid["noLeftBorderRadius"] || ""}
            disabled={false}/>
    </div>
);

export default OrderBy;
