import Dataset from "../../Dataset";
import TimeRangeInput from "../../TimeRangeInput";
import QueryInput from "../QueryInput";
import SearchButton from "../SearchButton";
import nativeStyles from "./index.module.css";


/**
 * Renders CLP-S controls.
 *
 * @return
 */
const ClpSControls = () => (
    <>
        <div className={nativeStyles["clpSRow"]}>
            <Dataset/>
            <div className={nativeStyles["clpSTimeRange"]}>
                <TimeRangeInput/>
            </div>
        </div>
        <div className={nativeStyles["clpSRow"]}>
            <div className={nativeStyles["clpSQuery"]}>
                <QueryInput/>
            </div>
            <SearchButton/>
        </div>
    </>
);

export default ClpSControls;
