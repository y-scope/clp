import useTimestampKeyInit from "../../../../SearchState/Presto/useTimestampKeyInit";
import searchStyles from "../../../index.module.css";
import QueryStatus from "../../../QueryStatus";
import TimeRangeInput from "../../../TimeRangeInput";
import SqlInterfaceSelector from "../../SqlInterfaceSelector";
import SqlSearchButton from "../../SqlSearchButton";
import From from "./From";
import guidedGrid from "./index.module.css";
import OrderBy from "./OrderBy";
import Select from "./Select";
import Where from "./Where";


/**
 * Renders controls and status for guided sql.
 *
 * @return
 */
const GuidedControls = () => {
    const {contextHolder} = useTimestampKeyInit();

    return (
        <div className={searchStyles["searchControlsContainer"]}>
            {contextHolder}
            <div className={searchStyles["runRow"]}>
                <div>
                    <SqlInterfaceSelector/>
                </div>
                <div className={searchStyles["buttons"]}>
                    <TimeRangeInput/>
                    <SqlSearchButton/>
                </div>
            </div>
            <div className={guidedGrid["gridContainer"]}>
                <Select/>
                <From/>
                <Where/>
                <OrderBy/>
            </div>
            <div className={searchStyles["status"]}>
                <QueryStatus/>
            </div>
        </div>
    );
};

export default GuidedControls;
