import {CLP_QUERY_ENGINES} from "@webui/common/config";

import {SETTINGS_QUERY_ENGINE} from "../../../config";
import usePrestoSearchState from "../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../SearchState/Presto/typings";
import NativeControls from "./Native/NativeControls";
import FreeformControls from "./Presto/Freeform/FreeformControls";
import GuidedControls from "./Presto/Guided/GuidedControls";


/**
 * Prevents the default behavior to avoid page reload when submitting query.
 *
 * @param ev
 */
const handleSubmit = (ev: React.FormEvent<HTMLFormElement>) => {
    ev.preventDefault();
};

/**
 * Renders controls for submitting queries and the query status.
 *
 * @return
 */
const SearchControls = () => {
    const sqlInterface = usePrestoSearchState((state) => state.sqlInterface);
    const isPrestoGuided = sqlInterface === PRESTO_SQL_INTERFACE.GUIDED;

    let controls;
    if (SETTINGS_QUERY_ENGINE !== CLP_QUERY_ENGINES.PRESTO) {
        controls = <NativeControls/>;
    } else if (isPrestoGuided) {
        controls = <GuidedControls/>;
    } else {
        controls = <FreeformControls/>;
    }

    return (
        <form onSubmit={handleSubmit}>
            {controls}
        </form>
    );
};

export default SearchControls;
