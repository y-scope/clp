import {useTracker} from "meteor/react-meteor-data";
import Col from "react-bootstrap/Col";
import Container from "react-bootstrap/Container";
import ProgressBar from "react-bootstrap/ProgressBar";
import Row from "react-bootstrap/Row";

import {StatsCollection} from "/imports/api/ingestion/collections";

import Details from "./panels/Details";
import SpaceSavings from "./panels/SpaceSavings";
import CompressionJobTable from "./CompressionJobTable";

import "./IngestView.scss";


/**
 * Presents compression statistics.
 *
 * @return {React.ReactElement}
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
                    {stats &&
                        <Row>
                            <SpaceSavings stats={stats}/>
                            <Details stats={stats}/>
                        </Row>}
                </Col>
            </Row>
            <Row>
                <CompressionJobTable/>
            </Row>
        </Container>
    );
};

export default IngestView;
