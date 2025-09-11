import QueryStatus from "../../QueryStatus";
import SqlInterfaceButton from "../SqlInterfaceButton";
import SqlSearchButton from "../SqlSearchButton";
import guidedGrid from "./index.module.css";
import searchStyles from "../../index.module.css";
import Select from "./Select";
import From from "./From";
import Where from "./Where";
import OrderBy from "./OrderBy";
import Limit from "./Limit";


/**
 * Renders controls and status for guided sql.
 *
 * @return
 */
const GuidedControls = () => (
    <div className={searchStyles["searchControlsContainer"]}>
        <div className={guidedGrid["gridContainer"]}>
            <Select />
            <From />
            <Where />
            <OrderBy />
            <Limit />
        </div>
        <div className={searchStyles["statusAndButtonsRow"]}>
            <div className={searchStyles["status"]}>
                <QueryStatus/>
            </div>
            <div className={searchStyles["buttons"]}>
                <SqlInterfaceButton/>
                <SqlSearchButton/>
            </div>
        </div>
    </div>
);

export default GuidedControls;
