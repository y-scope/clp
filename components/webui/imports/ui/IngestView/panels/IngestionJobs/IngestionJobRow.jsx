import OverlayTrigger from "react-bootstrap/OverlayTrigger";
import Spinner from "react-bootstrap/Spinner";
import Tooltip from "react-bootstrap/Tooltip";

import dayjs from "dayjs";

import {
    faCheck,
    faClock,
    faExclamation,
} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";

import {
    COMPRESSION_JOB_STATUS,
    COMPRESSION_JOB_STATUS_NAMES,
} from "/imports/api/ingestion/constants";
import {computeHumanSize} from "/imports/utils/misc";


/* eslint-disable line-comment-position, no-inline-comments, @stylistic/js/no-multi-spaces */
/**
 * Icons corresponding to different compression job statuses.
 *
 * @type {(import("@fortawesome/react-fontawesome").IconProp)[]}
 */
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
 * @property {Date} start_time
 * @property {number} duration
 * @property {number} uncompressed_size
 * @property {number} compressed_size
 */

/**
 * Renders an ingestion job.
 *
 * @param {CompressionJob} job The job object containing information about the compression job.
 * @return {React.ReactElement}
 */
const IngestionJobRow = ({job}) => {
    let speedText = "";
    let uncompressedSizeText = "";
    let compressedSizeText = "";

    if (null === job.duration && null !== job.start_time) {
        job.duration = dayjs.duration(
            dayjs() - dayjs(job.start_time)
        ).asSeconds();
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

export default IngestionJobRow;
