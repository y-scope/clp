import SqlInput from "../../../../components/SqlInput";
import QueryStatus from "../QueryStatus";
import SqlInterfaceButton from "./SqlInterfaceButton";
import SqlSearchButton from "./SqlSearchButton";
import { Select } from "antd";
import InputLabel from "../../../../components/InputLabel";
import guidedGridStyles from "./GuidedControls.module.css";
import Label from "./Label";
import styles from "../index.module.css";
import TimeRangeInput from "../TimeRangeInput";


const limitOptions = [
    { value: 10, label: "10" },
    { value: 50, label: "50" },
    { value: 100, label: "100" },
    { value: 500, label: "500" },
    { value: 1000, label: "1000" },
];

/**
 * Renders controls and status for guided sql.
 *
 * @return
 */
const GuidedControls = () => (
    <div className={styles["searchControlsContainer"]}>
        <div className={guidedGridStyles["gridContainer"]}>

            <div className={guidedGridStyles["select"]}>
                <InputLabel> SELECT </InputLabel>
                <SqlInput disabled={false} />
            </div>
            <div className={guidedGridStyles["from"]}>
                <InputLabel> FROM </InputLabel>
                <SqlInput disabled={false} />
            </div>
            <div className={guidedGridStyles["where"]}>
                <InputLabel> WHERE </InputLabel>
                <SqlInput disabled={false} />
            </div>
            <div className={guidedGridStyles["order"]}>
                <InputLabel> ORDER BY </InputLabel>
                <SqlInput disabled={false} />
            </div>
            <div className={guidedGridStyles["limit"]}>
                <InputLabel> LIMIT </InputLabel>
                    <Select
                        options={limitOptions}
                        defaultValue={limitOptions[0].value}
                        style={{ width: "100%" }}
                    >
                    </Select>
            </div>


        </div>
        <div className={styles["statusAndButtonsRow"]}>
            <div className={styles["status"]}>
                <QueryStatus/>
            </div>
            <div className={styles["buttons"]}>
                <SqlInterfaceButton/>
                <div className={guidedGridStyles["timestamp"]}>
                    <InputLabel> timestamp key </InputLabel>
                    <SqlInput disabled={false} />
                </div>
                <TimeRangeInput size="middle"/>
                <SqlSearchButton/>
            </div>
        </div>
    </div>
);

export default GuidedControls;
