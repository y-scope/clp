import dayjs from "dayjs";

import {
    faChartBar,
    faClock,
    faEnvelope,
    faFileAlt,
} from "@fortawesome/free-solid-svg-icons";

import {DATETIME_FORMAT_TEMPLATE} from "/imports/utils/datetime";

import Panel from "../../Panel";
import DetailsRow from "./DetailsRow";


/**
 * Presents details from the given statistics.
 *
 * @param {object} props
 * @param {CompressionStats} props.stats
 * @return {React.ReactElement}
 */
const Details = ({stats}) => {
    const {
        begin_timestamp: beginTimestamp,
        end_timestamp: endTimestamp,
        num_files: numFiles,
        num_messages: numMessages,
    } = stats;

    let timeRangeRow = null;
    if (null !== endTimestamp) {
        timeRangeRow = (
            <DetailsRow
                faIcon={faClock}
                title={"time range"}
            >
                <div>
                    {dayjs.utc(Number(beginTimestamp)).format(DATETIME_FORMAT_TEMPLATE)}
                    {" to"}
                </div>
                <div>
                    {dayjs.utc(Number(endTimestamp)).format(DATETIME_FORMAT_TEMPLATE)}
                </div>
            </DetailsRow>
        );
    }

    let numFilesRow = null;
    if (null !== numFiles) {
        numFilesRow = (
            <DetailsRow
                faIcon={faFileAlt}
                title={"files"}
            >
                {Number(numFiles).toLocaleString()}
            </DetailsRow>
        );
    }

    let numMessagesRow = null;
    if (null !== numMessages) {
        numMessagesRow = (
            <DetailsRow
                faIcon={faEnvelope}
                title={"messages"}
            >
                {Number(numMessages).toLocaleString()}
            </DetailsRow>
        );
    }

    if (!(timeRangeRow || numFilesRow || numMessagesRow)) {
        // No details to display
        return <></>;
    }

    return (
        <Panel
            faIcon={faChartBar}
            md={6}
            sm={12}
            title={"Details"}
        >
            {timeRangeRow}
            {numFilesRow}
            {numMessagesRow}
        </Panel>
    );
};

export default Details;
