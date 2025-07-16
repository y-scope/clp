import { useCallback } from "react";
import SqlEditor from "../../../../../components/SqlEditor";
import useSearchStore from "../../../SearchState/index";

/**
 * Renders SQL query input.
 */
const SqlQueryInput = () => {
    const handleChange = useCallback((value: string | undefined) => {
        const {updateQueryString} = useSearchStore.getState();
        updateQueryString(value || "");
    }, []);

    return (
        <SqlEditor
            height="150px"
            onChange={handleChange}
        />
    );
};

export default SqlQueryInput;
