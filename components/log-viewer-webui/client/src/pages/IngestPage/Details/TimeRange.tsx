import {Dayjs} from "dayjs";
import {Nullable} from "src/typings/common";

import DetailsCard from "./DetailsCard";


const DATE_FORMAT = "MMMM D, YYYY";

/**
 * Renders the time range statistic.
 *
 * @param props
 * @param props.startDate
 * @param props.endDate
 * @return
 */
const TimeRange = ({startDate, endDate}: {startDate: Nullable<Dayjs>;endDate: Nullable<Dayjs>}) => {
    const formattedStat = `${startDate?.format(DATE_FORMAT) ?? "N/A"} -
        ${endDate?.format(DATE_FORMAT) ?? "N/A"}`;

    return (
        <DetailsCard
            stat={formattedStat}
            title={"Time Range"}/>
    );
};

export default TimeRange;
