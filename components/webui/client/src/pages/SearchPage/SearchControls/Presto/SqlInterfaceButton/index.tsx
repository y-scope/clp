import usePrestoSearchState from "../../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../../SearchState/Presto/typings";
import FreeformButton from "./FreeformButton";
import GuidedButton from "./GuidedButton";
import styles from "./index.module.css";


/**
 * Renders the button to switch Presto SQL interface.
 *
 * @return
 */
const SqlInterfaceButton = () => {
    const sqlInterface = usePrestoSearchState((state) => state.sqlInterface);

    return (
        <div className={styles["sqlInterfaceButton"]}>
            {sqlInterface === PRESTO_SQL_INTERFACE.GUIDED ?
                <GuidedButton/> :
                <FreeformButton/>}
        </div>
    );
};

export default SqlInterfaceButton;
