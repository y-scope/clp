import InputLabel from "../../../../../../components/InputLabel";
import usePrestoSearchState from "../../../../SearchState/Presto";
import guidedGrid from "../index.module.css";
import TimestampKeySelect from "./TimestampKeySelect";


/**
 * Renders the timestamp key selector component.
 *
 * @return
 */
const TimestampKey = () => {
    const timestampKey = usePrestoSearchState((state) => state.timestampKey);
    const updateTimestampKey = usePrestoSearchState((state) => state.updateTimestampKey);

    return (
        <div className={guidedGrid["timestampKey"]}>
            <InputLabel>TIME KEY</InputLabel>
            <TimestampKeySelect
                className={
                    `${guidedGrid["noLeftBorderRadiusSelect"]} ${
                        guidedGrid["widthSelect"]}`
                }
                value={timestampKey}
                onChange={updateTimestampKey}
            />
        </div>
    );
};

export default TimestampKey;
