import {Meteor} from "meteor/meteor";
import {useTracker} from "meteor/react-meteor-data";
import React from "react";
import {Col, Container, ProgressBar, Row} from "react-bootstrap";

import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {faChartBar, faClock, faEnvelope, faFileAlt, faHdd} from "@fortawesome/free-solid-svg-icons";
import {DateTime} from "luxon";

import {StatsCollection} from "../../api/ingestion/publications";
import WidthFittedText from "../WidthFittedText/WidthFittedText.jsx";

const IngestView = ({isSidebarCollapsed}) => {
    // NOTE: We don't use stats_loaded since it only indicates whether the collection exists, not whether a stats document exists
    const [stats_loaded, stats] = useTracker(() => {
        const subscription = Meteor.subscribe(Meteor.settings.public.StatsCollectionName);
        let is_ready = subscription.ready();
        return [is_ready, StatsCollection.findOne({})];
    }, []);

    return (
        <Container fluid={true} className="ingest-container">
            <Row>
                <Col xs={12} xl={6}>
                    {stats ? (<Row>
                        <Col sm={12} md={6}>
                            <SpaceSavings stats={stats} widthDeps={[isSidebarCollapsed]} />
                        </Col>
                        <Col sm={12} md={6}>
                            <Details stats={stats} />
                        </Col>
                    </Row>) : (<></>)}
                </Col>
            </Row>
        </Container>
    );
}

const SpaceSavings = ({stats, widthDeps}) => {
    let logs_uncompressed_size = 0;
    let logs_compressed_size = 0;
    let spaceSavings = 0;
    if (stats) {
        logs_uncompressed_size = stats.total_uncompressed_size;
        logs_compressed_size = stats.total_compressed_size;
        if (0 === logs_uncompressed_size) {
            return (<></>);
        }
        spaceSavings = 100 - 100 * (logs_compressed_size / logs_uncompressed_size);
    }

    return (
        <div className="panel">
            <Row>
                <Col><h1 className="panel-h1">Space Savings</h1></Col>
                <Col xs="auto"><FontAwesomeIcon className="panel-icon" icon={faHdd} /></Col>
            </Row>
            <Row>
                <Col>
                    <WidthFittedText text={spaceSavings.toFixed(2) + "%"} widthDeps={widthDeps} />
                </Col>
            </Row>
            <Row>
                <Col>
                    <div className={"mb-2"}>
                        {prettify_storage_size(logs_uncompressed_size)} before compression
                        <ProgressBar now={100} className="ingest-stat-bar" />
                    </div>
                </Col>
            </Row>
            <Row>
                <Col>
                    <div className={"mb-2"}>
                        {prettify_storage_size(logs_compressed_size)} after compression
                        <ProgressBar now={100 - Math.round(spaceSavings)} className="ingest-stat-bar"/>
                    </div>
                </Col>
            </Row>
        </div>
    );
}

const Details = ({stats}) => {
    let timerange_begin_ts = 0;
    let timerange_end_ts = 0;
    let num_files = 0;
    let num_messages = 0;
    if (stats) {
        timerange_begin_ts = stats.begin_ts;
        timerange_end_ts = stats.end_ts;
        num_files = stats.num_files;
        num_messages = stats.num_messages;
    }

    let time_range_row = null;
    if (0 !== timerange_end_ts) {
        let timestamp_format = "kkkk-MMM-dd HH:mm";
        time_range_row = (
            <div className="ingest-stats-details-row">
                <div className="ingest-stats-details-icon-container">
                    <FontAwesomeIcon icon={faClock}/>
                </div>
                <div className="ingest-stats-details-text-container">
                    <span className="ingest-stats-detail">
                        {DateTime.fromMillis(timerange_begin_ts).toFormat(timestamp_format)}
                        <span className="ingest-desc-text"> to </span>
                        {DateTime.fromMillis(timerange_end_ts).toFormat(timestamp_format)}
                    </span>
                    <span className="ingest-desc-text">time range</span>
                </div>
            </div>
        );
    }

    let num_files_row = null;
    if (0 !== num_files) {
        num_files_row = (
            <div className="ingest-stats-details-row">
                <div className="ingest-stats-details-icon-container">
                    <FontAwesomeIcon icon={faFileAlt} />
                </div>
                <div className="ingest-stats-details-text-container">
                    <span className="ingest-stats-detail">{num_files.toLocaleString()}</span>
                    <span className="ingest-desc-text">files</span>
                </div>
            </div>
        );
    }

    let num_messages_row = null;
    if (0 !== num_messages) {
        num_messages_row = (
            <div className="ingest-stats-details-row">
                <div className="ingest-stats-details-icon-container">
                    <FontAwesomeIcon icon={faEnvelope} />
                </div>
                <div className="ingest-stats-details-text-container">
                    <span className="ingest-stats-detail">{num_messages.toLocaleString()}</span>
                    <span className="ingest-desc-text">messages</span>
                </div>
            </div>
        );
    }

    if (!(time_range_row || num_files_row || num_messages_row)) {
        // No details to display
        return (<></>);
    }

    return (
        <div className="panel">
            <Row>
                <Col><h1 className="panel-h1">Details</h1></Col>
                <Col xs="auto"><FontAwesomeIcon className="panel-icon" icon={faChartBar} /></Col>
            </Row>
            <Row>
                <Col>
                    {time_range_row}
                    {num_files_row}
                    {num_messages_row}
                </Col>
            </Row>
        </div>
    );
}

function prettify_storage_size (num) {
    const si_prefixes = ['', 'K', 'M', 'G', 'T', 'P', 'E', 'Z'];
    for (let i = 0; i < si_prefixes.length; ++i) {
        if (Math.abs(num) < 1024.0) {
            return "" + Math.round(num) + ' ' + si_prefixes[i] + "B";
        }
        num /= 1024.0;
    }
    return Math.round(num) + " B";
}

export default IngestView;
