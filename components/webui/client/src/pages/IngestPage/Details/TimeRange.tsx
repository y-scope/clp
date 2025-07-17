import {Dayjs} from "dayjs";

import {DashboardCard} from "../../../components/DashboardCard";
import Stat from "../../../components/Stat";


const DATE_FORMAT = "MMMM D, YYYY";

interface TimeRangeProps {
    beginDate: Dayjs;
    endDate: Dayjs;
    isLoading: boolean;
}

/**
 * Renders the time range statistic.
 *
 * @param props
 * @param props.beginDate
 * @param props.endDate
 * @param props.isLoading
 * @return
 */
const TimeRange = ({beginDate, endDate, isLoading}: TimeRangeProps) => {
    let stat;
    if (beginDate.isValid() && endDate.isValid()) {
        stat = `${beginDate.format(DATE_FORMAT)} - ${endDate.format(DATE_FORMAT)}`;
    } else {
        stat = "No timestamp data";
    }

    return (
        <DashboardCard
            isLoading={isLoading}
            title={"Time Range"}
        >
            <Stat text={stat}/>
        </DashboardCard>
    );
};

export default TimeRange;
