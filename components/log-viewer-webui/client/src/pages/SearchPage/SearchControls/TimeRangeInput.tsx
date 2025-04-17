import TimeRangeInputBase, {TIME_RANGE_OPTION} from "../../../components/TimeRangeInputBase";
import useSearchStore from "../SearchState";


/**
 * Renders an input to setting the time range filter for the query.
 *
 * @return
 */
const TimeRangeInput = () => {
    const updateTimeRange = useSearchStore((state) => state.updateTimeRange);

    return (
        <TimeRangeInputBase
            defaultValue={TIME_RANGE_OPTION.TODAY}
            onChange={updateTimeRange}/>
    );
};


export default TimeRangeInput;
