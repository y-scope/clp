import {useTracker} from "meteor/react-meteor-data";
import React from "react";
import {
    Col,
    Container,
    OverlayTrigger,
    ProgressBar,
    Row, Spinner,
    Table,
    Tooltip,
} from "react-bootstrap";

import {DateTime} from "luxon";

import {
    faBarsProgress,
    faChartBar,
    faCheck,
    faClock,
    faEnvelope,
    faExclamation,
    faFileAlt,
    faHdd,
} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";

import {
    CompressionJobsCollection,
    StatsCollection,
} from "/imports/api/ingestion/collections";
import {
    COMPRESSION_JOB_STATUS,
    COMPRESSION_JOB_STATUS_NAMES,
} from "/imports/api/ingestion/constants";
import {computeHumanSize} from "/imports/utils/misc";
import {MONGO_SORT_BY_ID} from "/imports/utils/mongo";

import CompressionJobTable from "./CompressionJobTable";


/**
 * Presents compression statistics.
 * @returns {JSX.Element}
 */
const IngestView = () => {
    const stats = useTracker(() => {
        Meteor.subscribe(Meteor.settings.public.StatsCollectionName);

        return StatsCollection.findOne();
    }, []);

    return (
        <Container
            className={"ingest-container"}
            fluid={true}
        >
            <Row>
                <Col
                    xl={6}
                    xs={12}
                >
                    {stats ?
                        (<Row>
                            <Col
                                md={6}
                                sm={12}
                            >
                                <SpaceSavings stats={stats}/>
                            </Col>
                            <Col
                                md={6}
                                sm={12}
                            >
                                <Details stats={stats}/>
                            </Col>
                        </Row>) :
                        (<></>)}
                </Col>
            </Row>
            <Row>
                <CompressionJobTable/>
            </Row>
        </Container>
    );
};

/**
 * Presents space savings from the given statistics.
 *
 * @param {Object} stats
 * @param {string} stats.total_uncompressed_size
 * @param {string} stats.total_compressed_size
 * @returns {JSX.Element}
 */
const SpaceSavings = ({stats}) => {
    const logsUncompressedSize = parseInt(stats.total_uncompressed_size) || 0;
    const logsCompressedSize = parseInt(stats.total_compressed_size) || 0;
    const spaceSavings = 0 < logsUncompressedSize ?
        100 * (1 - logsCompressedSize / logsUncompressedSize) :
        0;

    return (
        <div className={"panel"}>
            <Row>
                <Col>
                    <h1 className={"panel-h1"}>Space Savings</h1>
                </Col>
                <Col xs={"auto"}>
                    <FontAwesomeIcon
                        className={"panel-icon"}
                        icon={faHdd}/>
                </Col>
            </Row>
            <Row>
                <Col>
                    <svg viewBox={"0 0 48 12"}>
                        <text
                            x={"0"}
                            y={"10"}
                        >
                            {`${spaceSavings.toFixed(2)}%`}
                        </text>
                    </svg>
                </Col>
            </Row>
            <Row>
                <Col>
                    <div className={"mb-2"}>
                        {computeHumanSize(logsUncompressedSize)}
                        {" "}
                        before compression
                        <ProgressBar
                            className={"ingest-stat-bar"}
                            now={100}/>
                    </div>
                </Col>
            </Row>
            <Row>
                <Col>
                    <div className={"mb-2"}>
                        {computeHumanSize(logsCompressedSize)}
                        {" "}
                        after compression
                        <ProgressBar
                            className={"ingest-stat-bar"}
                            now={100 - Math.round(spaceSavings)}/>
                    </div>
                </Col>
            </Row>
        </div>
    );
};

/**
 * Presents details from the given statistics.
 *
 * @param {Object|null} stats - The statistics object.
 * @param {number|null} stats.begin_timestamp
 * @param {number|null} stats.end_timestamp
 * @param {number|null} stats.num_files
 * @param {number|null} stats.num_messages
 * @returns {JSX.Element}
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
        const timestampFormat = "kkkk-MMM-dd HH:mm";
        timeRangeRow = (
            <div className={"ingest-stats-details-row"}>
                <div className={"ingest-stats-details-icon-container"}>
                    <FontAwesomeIcon icon={faClock}/>
                </div>
                <div className={"ingest-stats-details-text-container"}>
                    <span className={"ingest-stats-detail"}>
                        {DateTime.fromMillis(Number(beginTimestamp)).toFormat(timestampFormat)}
                        <span className={"ingest-desc-text"}> to </span>
                        {DateTime.fromMillis(Number(endTimestamp)).toFormat(timestampFormat)}
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

export default IngestView;
