import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import guidedGrid from "./index.module.css";


/**
 * Renders the FROM SQL input field.
 *
 * @return
 */
const From = () => (
    <div className={guidedGrid["from"]}>
        <InputLabel> FROM </InputLabel>
        <SqlInput disabled={false} />
    </div>
);

export default From;
