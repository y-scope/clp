import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import guidedGridStyles from "./index.module.css";

const Where = () => (
    <div className={guidedGridStyles["where"]}>
        <InputLabel> WHERE </InputLabel>
        <SqlInput disabled={false} />
    </div>
);

export default Where;
