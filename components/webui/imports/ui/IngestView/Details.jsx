import {
    Col,
    Row,
} from "react-bootstrap";

import dayjs from "dayjs";

import {
    faChartBar,
    faClock,
    faEnvelope,
    faFileAlt,
} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";

import {DATETIME_FORMAT_TEMPLATE} from "/imports/utils/datetime";


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
            <div className={"ingest-stats-details-row"}>
                <div className={"ingest-stats-details-icon-container"}>
                    <FontAwesomeIcon icon={faClock}/>
                </div>
                <div className={"ingest-stats-details-text-container"}>
                    <span className={"ingest-stats-detail"}>
                        <div>
                            {dayjs.utc(Number(beginTimestamp)).format(DATETIME_FORMAT_TEMPLATE)}
                            {" to"}
                        </div>
                        <div>
                            {dayjs.utc(Number(endTimestamp)).format(DATETIME_FORMAT_TEMPLATE)}
                        </div>
                    </span>
                    <span className={"ingest-desc-text"}>time range</span>
                </div>
            </div>
        );
    }

    let numFilesRow = null;
    if (null !== numFiles) {
        numFilesRow = (
            <div className={"ingest-stats-details-row"}>
                <div className={"ingest-stats-details-icon-container"}>
                    <FontAwesomeIcon icon={faFileAlt}/>
                </div>
                <div className={"ingest-stats-details-text-container"}>
                    <span className={"ingest-stats-detail"}>
                        {Number(numFiles).toLocaleString()}
                    </span>
                    <span className={"ingest-desc-text"}>files</span>
                </div>
            </div>
        );
    }

    let numMessagesRow = null;
    if (null !== numMessages) {
        numMessagesRow = (
            <div className={"ingest-stats-details-row"}>
                <div className={"ingest-stats-details-icon-container"}>
                    <FontAwesomeIcon icon={faEnvelope}/>
                </div>
                <div className={"ingest-stats-details-text-container"}>
                    <span
                        className={"ingest-stats-detail"}
                    >
                        {Number(numMessages).toLocaleString()}
                    </span>
                    <span className={"ingest-desc-text"}>messages</span>
                </div>
            </div>
        );
    }

    if (!(timeRangeRow || numFilesRow || numMessagesRow)) {
        // No details to display
        return (<></>);
    }

    return (
        <div className={"panel"}>
            <Row>
                <Col>
                    <h1 className={"panel-h1"}>Details</h1>
                </Col>
                <Col xs={"auto"}>
                    <FontAwesomeIcon
                        className={"panel-icon"}
                        icon={faChartBar}/>
                </Col>
            </Row>
            <Row>
                <Col>
                    {timeRangeRow}
                    {numFilesRow}
                    {numMessagesRow}
                </Col>
            </Row>
        </div>
    );
};

export default Details;
