import searchStyles from "../../index.module.css";
import QueryStatus from "../../QueryStatus";
import TimeRangeInput from "../../TimeRangeInput";
import SqlInterfaceButton from "../SqlInterfaceButton";
import SqlSearchButton from "../SqlSearchButton";
import From from "./From";
import guidedGrid from "./index.module.css";
import Limit from "./Limit";
import OrderBy from "./OrderBy";
import Select from "./Select";
import TimestampKey from "./TimestampKey";
import Where from "./Where";


/**
 * Renders controls and status for guided sql.
 *
 * @return
 */
const GuidedControls = () => (
    <div className={searchStyles["searchControlsContainer"]}>
        <div className={guidedGrid["gridContainer"]}>
            <Select/>
            <From/>
            <TimestampKey/>
            <Where/>
            <OrderBy/>
            <Limit/>
        </div>
        <div className={searchStyles["statusAndButtonsRow"]}>
            <div className={searchStyles["status"]}>
                <QueryStatus/>
            </div>
            <div className={searchStyles["buttons"]}>
                <TimeRangeInput/>
                <SqlInterfaceButton/>
                <SqlSearchButton/>
            </div>
        </div>
    </div>
);

export default GuidedControls;
