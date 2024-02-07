import {faChartBar, faClock, faEnvelope, faFileAlt, faHdd} from "@fortawesome/free-solid-svg-icons";

import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {DateTime} from "luxon";
import {Meteor} from "meteor/meteor";
import {useTracker} from "meteor/react-meteor-data";
import React, {useEffect} from "react";
import {Col, Container, ProgressBar, Row} from "react-bootstrap";

import {StatsCollection} from "../../api/ingestion/collections";
import {computeHumanSize} from "../../utils/misc";


/**
 * Presents compression statistics.
 * @returns {JSX.Element}
 */
const IngestView = () => {
    const stats = useTracker(() => {
        return StatsCollection.findOne();
    }, []);

    useEffect(() => {
        let subscription = Meteor.subscribe(Meteor.settings.public.StatsCollectionName);
        const interval = Meteor.setInterval(() => {
            const oldSubscription = subscription;
            subscription = Meteor.subscribe(Meteor.settings.public.StatsCollectionName);
            oldSubscription.stop();
        }, 5000);

        return () => {
            Meteor.clearInterval(interval);
        };
    }, []);

    return (
        <Container fluid={true} className="ingest-container">
            <Row>
                <Col xs={12} xl={6}>
                    {stats ? (<Row>
                        <Col sm={12} md={6}>
                            <SpaceSavings stats={stats}/>
                        </Col>
                        <Col sm={12} md={6}>
                            <Details stats={stats}/>
                        </Col>
                    </Row>) : (<></>)}
                </Col>
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
    const spaceSavings = logsUncompressedSize > 0 ? 100 * (1 - logsCompressedSize / logsUncompressedSize) : 0;

    return (
        <div className="panel">
            <Row>
                <Col><h1 className="panel-h1">Space Savings</h1></Col>
                <Col xs="auto"><FontAwesomeIcon className="panel-icon" icon={faHdd}/></Col>
            </Row>
            <Row>
                <Col>
                    <svg viewBox="0 0 48 12">
                        <text x="0" y="10">{spaceSavings.toFixed(2) + "%"}</text>
                    </svg>
                </Col>
            </Row>
            <Row>
                <Col>
                    <div className={"mb-2"}>
                        {computeHumanSize(logsUncompressedSize)} before compression
                        <ProgressBar now={100} className="ingest-stat-bar"/>
                    </div>
                </Col>
            </Row>
            <Row>
                <Col>
                    <div className={"mb-2"}>
                        {computeHumanSize(logsCompressedSize)} after compression
                        <ProgressBar now={100 - Math.round(spaceSavings)}
                                     className="ingest-stat-bar"/>
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
        begin_timestamp: beginTimeStamp,
        end_timestamp: endTimeStamp,
        num_files: numFiles,
        num_messages: numMessages
    } = stats;

    let timeRangeRow = null;
    if (null !== endTimeStamp) {
        let timestamp_format = "kkkk-MMM-dd HH:mm";
        timeRangeRow = (
            <div className="ingest-stats-details-row">
                <div className="ingest-stats-details-icon-container">
                    <FontAwesomeIcon icon={faClock}/>
                </div>
                <div className="ingest-stats-details-text-container">
                    <span className="ingest-stats-detail">
                        {DateTime.fromMillis(beginTimeStamp).toFormat(timestamp_format)}
                        <span className="ingest-desc-text"> to </span>
                        {DateTime.fromMillis(endTimeStamp).toFormat(timestamp_format)}
                    </span>
                    <span className="ingest-desc-text">time range</span>
                </div>
            </div>
        );
    }

    let num_files_row = null;
    if (null !== numFiles) {
        num_files_row = (
            <div className="ingest-stats-details-row">
                <div className="ingest-stats-details-icon-container">
                    <FontAwesomeIcon icon={faFileAlt}/>
                </div>
                <div className="ingest-stats-details-text-container">
                    <span className="ingest-stats-detail">{numFiles.toLocaleString()}</span>
                    <span className="ingest-desc-text">files</span>
                </div>
            </div>
        );
    }

    let num_messages_row = null;
    if (null !== numMessages) {
        num_messages_row = (
            <div className="ingest-stats-details-row">
                <div className="ingest-stats-details-icon-container">
                    <FontAwesomeIcon icon={faEnvelope}/>
                </div>
                <div className="ingest-stats-details-text-container">
                    <span className="ingest-stats-detail">{numMessages.toLocaleString()}</span>
                    <span className="ingest-desc-text">messages</span>
                </div>
            </div>
        );
    }

    if (!(timeRangeRow || num_files_row || num_messages_row)) {
        // No details to display
        return (<></>);
    }

    return (
        <div className="panel">
            <Row>
                <Col><h1 className="panel-h1">Details</h1></Col>
                <Col xs="auto"><FontAwesomeIcon className="panel-icon" icon={faChartBar}/></Col>
            </Row>
            <Row>
                <Col>
                    {timeRangeRow}
                    {num_files_row}
                    {num_messages_row}
                </Col>
            </Row>
        </div>
    );
};

export default IngestView;
