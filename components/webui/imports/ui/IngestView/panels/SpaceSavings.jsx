import Col from "react-bootstrap/Col";
import ProgressBar from "react-bootstrap/ProgressBar";
import Row from "react-bootstrap/Row";

import {faHdd} from "@fortawesome/free-solid-svg-icons";

import {computeHumanSize} from "/imports/utils/misc";

import Panel from "../Panel";


/**
 * Presents space savings from the given statistics.
 *
 * @param {object} props
 * @param {CompressionStats} props.stats
 * @return {React.ReactElement}
 */
const SpaceSavings = ({stats}) => {
    const logsUncompressedSize = parseInt(stats.total_uncompressed_size, 10) || 0;
    const logsCompressedSize = parseInt(stats.total_compressed_size, 10) || 0;
    const spaceSavings = 0 < logsUncompressedSize ?
        100 * (1 - (logsCompressedSize / logsUncompressedSize)) :
        0;

    return (
        <Panel
            faIcon={faHdd}
            md={6}
            sm={12}
            title={"Space Savings"}
        >
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
        </Panel>
    );
};

export default SpaceSavings;
