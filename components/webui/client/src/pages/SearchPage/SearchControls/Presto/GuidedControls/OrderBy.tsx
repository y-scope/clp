import InputLabel from "../../../../../components/InputLabel";
import SqlInput from "../../../../../components/SqlInput";
import usePrestoSearchState from "../../../SearchState/Presto";
import guidedGrid from "./index.module.css";


/**
 * Renders the ORDER BY SQL input field.
 *
 * @return
 */
const OrderBy = () => {
    const orderBy = usePrestoSearchState((state) => state.orderBy);
    const updateOrderBy = usePrestoSearchState((state) => state.updateOrderBy);

    return (
        <div className={guidedGrid["order"]}>
            <InputLabel>ORDER BY</InputLabel>
            <SqlInput
                className={guidedGrid["noLeftBorderRadius"] || ""}
                disabled={false}
                value={orderBy}
                onChange={(value) => {
                    updateOrderBy(value || "");
                }}/>
        </div>
    );
};

export default OrderBy;
