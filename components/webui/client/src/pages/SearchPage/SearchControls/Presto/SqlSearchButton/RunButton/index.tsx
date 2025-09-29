import usePrestoSearchState from "../../../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../../../SearchState/Presto/typings";

import FreeformRunButton from "./FreeformRunButton";
import GuidedRunButton from "./GuidedRunButton";


/**
 * Renders a button to run the SQL query based on the current interface mode.
 *
 * @return
 */
const RunButton = () => {
    const sqlInterface = usePrestoSearchState((state) => state.sqlInterface);

    return sqlInterface === PRESTO_SQL_INTERFACE.GUIDED ?
        <GuidedRunButton/> :
        <FreeformRunButton/>;
};

export default RunButton;