import useIngestStore from "../IngestState";
import DetailsCard from "./DetailsCard";


const DATE_FORMAT = "MMMM D, YYYY";

/**
 * Renders the time range statistic.
 *
 * @return
 */
const TimeRange = () => {
    const [startDate, endDate] = useIngestStore((state) => state.timeRange);
    const formattedStat = `${startDate.format(DATE_FORMAT)} -
        ${endDate.format(DATE_FORMAT)}`;

    return (
        <DetailsCard
            stat={formattedStat}
            title={"Time Range"}/>
    );
};

export default TimeRange;
