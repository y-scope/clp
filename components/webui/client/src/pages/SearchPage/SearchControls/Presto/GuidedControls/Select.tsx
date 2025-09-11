import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import guidedGridStyles from "./index.module.css";

const Select = () => (
    <div className={guidedGridStyles["select"]}>
        <InputLabel> SELECT </InputLabel>
        <SqlInput disabled={false} />
    </div>
);

export default Select;
