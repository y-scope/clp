import {Select as AntdSelect} from "antd";

import InputLabel from "../../../../../components/InputLabel";
import guidedGrid from "./index.module.css";


const LIMIT_OPTIONS = [
    {value: 10, label: "10"},
    {value: 50, label: "50"},
    {value: 100, label: "100"},
    {value: 500, label: "500"},
    {value: 1000, label: "1000"},
];

/**
 * Renders the LIMIT SQL input field.
 *
 * @return
 */
const Limit = () => (
    <div className={guidedGrid["limit"]}>
        <InputLabel> LIMIT </InputLabel>
        <AntdSelect
            defaultValue={LIMIT_OPTIONS[0]?.value}
            options={LIMIT_OPTIONS}
            style={{width: "100%"}}/>
    </div>
);

export default Limit;
