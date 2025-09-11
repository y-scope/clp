import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import guidedGrid from "./index.module.css";

const OrderBy = () => (
    <div className={guidedGrid["order"]}>
        <InputLabel> ORDER BY </InputLabel>
        <SqlInput disabled={false} />
    </div>
);

export default OrderBy;
