import InputLabel from "../../../../../../components/InputLabel";
import guidedGrid from "../index.module.css";
import TimestampKeySelect from "./TimestampKeySelect";


/**
 * Renders the timestamp key selector component.
 *
 * @return
 */
const TimestampKey = () => (
    <div className={guidedGrid["timestampKey"]}>
        <InputLabel>TIME KEY</InputLabel>
        <TimestampKeySelect
            className={
                `${guidedGrid["noLeftBorderRadiusSelect"]} ${
                    guidedGrid["widthSelect"]}`
            }/>
    </div>
);

export default TimestampKey;
