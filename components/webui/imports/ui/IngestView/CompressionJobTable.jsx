import {useTracker} from "meteor/react-meteor-data";
import React from "react";
import {
    Col, OverlayTrigger, Row, Spinner, Table, Tooltip,
} from "react-bootstrap";

import {
    faBarsProgress, faCheck, faClock, faExclamation,
} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";

import {CompressionJobsCollection} from "/imports/api/ingestion/collections";
import {
    COMPRESSION_JOB_STATUS,
    COMPRESSION_JOB_STATUS_NAMES,
} from "/imports/api/ingestion/constants";
import {computeHumanSize} from "/imports/utils/misc";
import {MONGO_SORT_BY_ID} from "/imports/utils/mongo";


/* eslint-disable line-comment-position, no-inline-comments, @stylistic/js/no-multi-spaces */
const COMPRESSION_JOB_STATUS_ICONS = Object.freeze([
    faClock,        // PENDING
    null,           // RUNNING: <Spinner/> is shown instead
    faCheck,        // SUCCEEDED
    faExclamation,  // FAILED
]);
/* eslint-enable line-comment-position, no-inline-comments, @stylistic/js/no-multi-spaces */

/**
 * @typedef {object} CompressionJob
 * @property {number} _id
 * @property {number} status
 * @property {string} status_msg
 * @property {string} start_time
 * @property {number} duration
 * @property {number} uncompressed_size
 * @property {number} compressed_size
 */

/**
 * Renders a compression job.
 * @param {CompressionJob} job - The job object containing information about the compression job.
 * @return {JSX.Element}
 */
const CompressionJobRow = ({job}) => {
    let speedText = "";
    let uncompressedSizeText = "";
    let compressedSizeText = "";
    if (null === job.duration && null !== job.start_time) {
        job.duration = (Date.now() - job.start_time) / 1000;
    }
    if (0 < job.duration) {
        speedText = `${computeHumanSize(job.uncompressed_size / job.duration)}/s`;
        uncompressedSizeText = computeHumanSize(job.uncompressed_size);
        compressedSizeText = computeHumanSize(job.compressed_size);
    }

    return (
        <tr>
            <td className={"text-center"}>
                <OverlayTrigger
                    placement={"bottom-end"}
                    overlay={
                        <Tooltip>
                            {COMPRESSION_JOB_STATUS_NAMES[job.status]}
                            {job.status_msg && (` - ${job.status_msg}`)}
                        </Tooltip>
                    }
                >
                    {COMPRESSION_JOB_STATUS.RUNNING === job.status ?
                        <Spinner size={"sm"}/> :
                        <FontAwesomeIcon
                            fixedWidth={true}
                            icon={COMPRESSION_JOB_STATUS_ICONS[job.status]}/>}
                </OverlayTrigger>
            </td>
            <td className={"fw-bold"}>
                {job._id}
            </td>
            <td className={"text-right"}>
                {speedText}
            </td>
            <td className={"text-right"}>
                {uncompressedSizeText}
            </td>
            <td className={"text-right"}>
                {compressedSizeText}
            </td>
        </tr>
    );
};

/**
 * Displays a table of compression jobs.
 * @return {JSX.Element}
 */
const CompressionJobTable = () => {
    const compressionJobs = useTracker(() => {
        Meteor.subscribe(Meteor.settings.public.CompressionJobsCollectionName);

        const findOptions = {
            sort: [MONGO_SORT_BY_ID],
        };

        return CompressionJobsCollection.find({}, findOptions).fetch();
    }, []);

    if (0 === compressionJobs.length) {
        return <></>;
    }

    return (
        <Col
            xl={6}
            xs={12}
        >
            <div className={"panel"}>
                <Row>
                    <Col>
                        <h1 className={"panel-h1"}>Ingestion Jobs</h1>
                    </Col>
                    <Col xs={"auto"}>
                        <FontAwesomeIcon
                            className={"panel-icon"}
                            icon={faBarsProgress}/>
                    </Col>
                </Row>
                <Row>
                    <Col>
                        <Table>
                            <thead>
                                <tr>
                                    <th className={"text-center"}>Status</th>
                                    <th>Job ID</th>
                                    <th className={"text-right"}>Speed</th>
                                    <th className={"text-right"}>Data Ingested</th>
                                    <th className={"text-right"}>Compressed Size</th>
                                </tr>
                            </thead>
                            <tbody>
                                {compressionJobs.map((job, i) => (
                                    <CompressionJobRow
                                        job={job}
                                        key={i}/>
                                ))}
                            </tbody>
                        </Table>
                    </Col>
                </Row>
            </div>
        </Col>
    );
};
export default CompressionJobTable;
