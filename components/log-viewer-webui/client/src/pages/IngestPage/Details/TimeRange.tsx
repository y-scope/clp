import {Dayjs} from "dayjs";
import {Nullable} from "src/typings/common";

import DetailsCard from "./DetailsCard";


const DATE_FORMAT = "MMMM D, YYYY";

interface TimeRangeProps {
    beginDate: Nullable<Dayjs>;
    endDate: Nullable<Dayjs>;
}

/**
 * Renders the time range statistic.
 *
 * @param props
 * @param props.beginDate
 * @param props.endDate
 * @return
 */
const TimeRange = ({beginDate, endDate}: TimeRangeProps) => {
    const formattedStat = `${beginDate?.format(DATE_FORMAT) ?? "Unknown Begin Date"} -
        ${endDate?.format(DATE_FORMAT) ?? "Unknown End Date"}`;

    return (
        <DetailsCard
            stat={formattedStat}
            title={"Time Range"}/>
    );
};

export default TimeRange;
