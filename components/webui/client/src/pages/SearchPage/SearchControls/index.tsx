import {
    CLP_QUERY_ENGINES,
    SETTINGS_QUERY_ENGINE,
} from "../../../config";
import usePrestoSearchState from "../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../SearchState/Presto/typings";
import NativeControls from "./NativeControls";
import FreeformControls from "./Presto/FreeformControls";
import GuidedControls from "./Presto/GuidedControls";


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
    /* eslint-disable-next-line no-warning-comments */
    // TODO: Remove flag and related logic when the new guide UI is fully implemented.
    const isGuidedEnabled = "true" === import.meta.env["VITE_GUIDED_DEV"];

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
