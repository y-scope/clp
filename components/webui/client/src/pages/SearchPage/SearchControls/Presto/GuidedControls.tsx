import SqlInput from "../../../../components/SqlInput";
import QueryStatus from "../QueryStatus";
import SqlInterfaceButton from "./SqlInterfaceButton";
import SqlSearchButton from "./SqlSearchButton";
import { Select } from "antd";
import InputLabel from "../../../../components/InputLabel";
import guidedGridStyles from "./GuidedControls.module.css";
import Label from "./Label";


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
    <div className={guidedGridStyles["gridContainer"]}>

        <div className={guidedGridStyles["select"]}>
             <InputLabel> SELECT </InputLabel>
              <SqlInput disabled={false} />
        </div>
        <div className={guidedGridStyles["dataset"]}>
            <InputLabel> FROM </InputLabel>
            <SqlInput disabled={false} />
        </div>
        <div className={guidedGridStyles["where"]}>
            <InputLabel> WHERE </InputLabel>
            <SqlInput disabled={false} />
        </div>
        <div className={guidedGridStyles["order"]}>
            <Label>ORDER BY</Label>
            <div style={{ flex: 1 }}>
                <SqlInput disabled={false} />
            </div>
        </div>

        <div className={guidedGridStyles["limit"]}>
            <Label>LIMIT</Label>
            <div style={{ flex: 1 }}>
                <Select
                    options={limitOptions}
                    defaultValue={limitOptions[0].value}
                    style={{ width: "100%" }}
                />
            </div>
        </div>

        <div className={guidedGridStyles["status"]}>
            <QueryStatus />
        </div>
        <div className={guidedGridStyles["buttons"]}>
            <SqlInterfaceButton />
            <SqlSearchButton />
        </div>
    </div>
);

export default GuidedControls;
