import {theme} from "antd";
import dayjs from "dayjs";

import StatCard from "../../../components/StatCard";


// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_START_DATE = "2021-12-14";
const DUMMY_END_DATE = "2025-04-16";

const DATE_FORMAT = "MMMM D, YYYY";

/**
 * Renders the time range statistic.
 *
 * @return
 */
const TimeRange = () => {
    const {token} = theme.useToken();
    return (
        <StatCard
            statColor={token.colorTextSecondary}
            statSize={"1.3rem"}
            title={"Time Range"}
            stat={`${dayjs(DUMMY_START_DATE).format(DATE_FORMAT)} -
             ${dayjs(DUMMY_END_DATE).format(DATE_FORMAT)}`}/>
    );
};

export default TimeRange;
