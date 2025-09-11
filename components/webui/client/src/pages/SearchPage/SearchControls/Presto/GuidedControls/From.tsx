import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import guidedGridStyles from "./index.module.css";

const From = () => (
    <div className={guidedGridStyles["from"]}>
        <InputLabel> FROM </InputLabel>
        <SqlInput disabled={false} />
    </div>
);

export default From;
