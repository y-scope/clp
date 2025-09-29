import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import usePrestoSearchState from "../../../SearchState/Presto";
import guidedGrid from "./index.module.css";


/**
 * Renders the SELECT SQL input field.
 *
 * @return
 */
const Select = () => {
    const select = usePrestoSearchState((state) => state.select);
    const updateSelect = usePrestoSearchState((state) => state.updateSelect);

    return (
        <div className={guidedGrid["select"]}>
            <InputLabel>SELECT</InputLabel>
            <SqlInput
                className={guidedGrid["noLeftBorderRadius"] || ""}
                disabled={false}
                value={select}
                onChange={(value) => {
                    updateSelect(value || "");
                }}/>
        </div>
    );
};

export default Select;
