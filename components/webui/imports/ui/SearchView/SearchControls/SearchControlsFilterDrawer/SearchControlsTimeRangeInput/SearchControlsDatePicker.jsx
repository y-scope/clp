import DatePicker from "react-datepicker";

import "react-datepicker/dist/react-datepicker.css";
import "./SearchControlsTimeRangeInput.scss";


/**
 * Renders a date picker control for selecting date and time.
 *
 * @param {object} props
 * @return {React.ReactElement}
 */
const SearchControlsDatePicker = (props) => (
    <DatePicker
        {...props}
        className={"timestamp-picker"}
        dateFormat={"MMM d, yyyy h:mm aa"}
        dropdownMode={"select"}
        showTimeSelect={true}
        timeCaption={"Time"}
        timeFormat={"HH:mm"}
        timeIntervals={15}/>
);

export default SearchControlsDatePicker;
