import {useTracker} from "meteor/react-meteor-data";
import {
    Col,
    Container,
    Row,
} from "react-bootstrap";

import {StatsCollection} from "/imports/api/ingestion/collections";

import Details from "./Details";
import SpaceSavings from "./SpaceSavings";


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
        </Container>
    );
};

export default IngestView;
