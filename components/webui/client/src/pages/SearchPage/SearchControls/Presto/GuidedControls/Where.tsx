import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import usePrestoSearchState from "../../../SearchState/Presto";
import guidedGrid from "./index.module.css";


/**
 * Renders the WHERE SQL input field.
 *
 * @return
 */
const Where = () => {
    const where = usePrestoSearchState((state) => state.where);
    const updateWhere = usePrestoSearchState((state) => state.updateWhere);

    return (
        <div className={guidedGrid["where"]}>
            <InputLabel>WHERE</InputLabel>
            <SqlInput
                className={guidedGrid["noLeftBorderRadius"] || ""}
                disabled={false}
                value={where}
                onChange={(value) => {
                    updateWhere(value || "");
                }}/>
        </div>
    );
};

export default Where;
