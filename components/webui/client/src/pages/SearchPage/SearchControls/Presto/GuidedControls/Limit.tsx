import {Select} from "antd";

import InputLabel from "../../../../../components/InputLabel";
import usePrestoSearchState from "../../../SearchState/Presto";
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
const Limit = () => {
    const limit = usePrestoSearchState((state) => state.limit);
    const updateLimit = usePrestoSearchState((state) => state.updateLimit);

    return (
        <div className={guidedGrid["limit"]}>
            <InputLabel>LIMIT</InputLabel>
            <Select
                value={limit}
                options={LIMIT_OPTIONS}
                onChange={updateLimit}
                className={
                    `${guidedGrid["noLeftBorderRadiusSelect"]} ${
                        guidedGrid["widthSelect"]}`
                }
            />
        </div>
    );
};

export default Limit;
