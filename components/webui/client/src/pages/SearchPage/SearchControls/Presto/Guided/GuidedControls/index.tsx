import useTimestampKeyInit from "../../../../SearchState/Presto/useTimestampKeyInit";
import searchStyles from "../../../index.module.css";
import InputLabel from "../../../../../../components/InputLabel";
import DatasetSelect from "../../../Dataset/DatasetSelect";
import QueryStatus from "../../../QueryStatus";
import TimeRangeInput from "../../../TimeRangeInput";
import SqlSearchButton from "../../SqlSearchButton";
import guidedGrid from "./index.module.css";
import OrderBy from "./OrderBy";
import Select from "./Select";
import Where from "./Where";
import {LABEL_WIDTH} from "./typings";


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
            <div className={guidedGrid["datasetRow"]}>
                <div className={guidedGrid["datasetContainer"]}>
                    <InputLabel width={LABEL_WIDTH}>DATASET</InputLabel>
                    <DatasetSelect
                        className={`${guidedGrid["datasetSelect"]} ${guidedGrid["noLeftBorderRadiusSelect"]}`}/>
                </div>
                <div className={guidedGrid["timeRangeContainer"]}>
                    <TimeRangeInput/>
                </div>
            </div>
            <div className={guidedGrid["inputsAndRunRow"]}>
                <div className={guidedGrid["gridContainer"]}>
                    <Select/>
                    <Where/>
                    <OrderBy/>
                </div>
                <div className={guidedGrid["runColumn"]}>
                    <SqlSearchButton/>
                </div>
            </div>
            <div className={searchStyles["status"]}>
                <QueryStatus/>
            </div>
        </div>
    );
};

export default GuidedControls;
