import InputLabel from "../../../../../components/InputLabel";
import usePrestoSearchState from "../../../SearchState/Presto";
import DatasetSelect from "../../Dataset/DatasetSelect";
import guidedGrid from "./index.module.css";


/**
 * Renders the FROM SQL input field.
 *
 * @return
 */
const From = () => {
    const from = usePrestoSearchState((state) => state.from);
    const updateFrom = usePrestoSearchState((state) => state.updateFrom);

    return (
        <div className={guidedGrid["from"]}>
            <InputLabel>FROM</InputLabel>
            <DatasetSelect
                className={
                    `${guidedGrid["noLeftBorderRadiusSelect"]} ${
                        guidedGrid["widthSelect"]}`
                }
                value={from}
                onChange={updateFrom}
            />
        </div>
    );
};

export default From;
