import InputLabel from "../../../../../components/InputLabel";
import { Select as AntdSelect } from "antd";
import guidedGridStyles from "./index.module.css";

const limitOptions = [
    { value: 10, label: "10" },
    { value: 50, label: "50" },
    { value: 100, label: "100" },
    { value: 500, label: "500" },
    { value: 1000, label: "1000" },
];

const Limit = () => (
    <div className={guidedGridStyles["limit"]}>
        <InputLabel> LIMIT </InputLabel>
        <AntdSelect
            options={limitOptions}
            defaultValue={limitOptions[0].value}
            style={{ width: "100%" }}
        />
    </div>
);

export default Limit;
