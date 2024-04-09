import {
    Button,
    Col,
    Container,
    Form,
    InputGroup,
    OverlayTrigger,
    Popover,
    Row,
} from "react-bootstrap";

import {faCog} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";

import "./SearchResultsHeader.scss";


/**
 * Renders the header for the search results, which includes the search job ID, the number of
 * results found, and a control for setting the maximum number of lines per search result.
 *
 * @param {object} props
 * @param {number} props.maxLinesPerResult
 * @param {number} props.numResultsOnServer
 * @param {number} props.searchJobId
 * @param {Function} props.setMaxLinesPerResult
 * @return {JSX.Element}
 */
const SearchResultsHeader = ({
    maxLinesPerResult,
    numResultsOnServer,
    searchJobId,
    setMaxLinesPerResult,
}) => {
    const handleMaxLinesPerResultSubmission = (e) => {
        e.preventDefault();
        const value = parseInt(e.target.elements.maxLinesPerResult.value);
        if (0 < value) {
            setMaxLinesPerResult(value);
        }
    };

    return (
        <>
            <Container fluid={true}>
                <Row className={"search-results-title-bar"}>
                    <Col>
                        <span className={"search-results-count"}>
                            Job ID
                            {" "}
                            {searchJobId}
                            {" "}
                            | Results count:
                            {" "}
                            {numResultsOnServer}
                        </span>
                    </Col>
                    <Col xs={"auto"}>
                        <OverlayTrigger
                            placement={"left"}
                            trigger={"click"}
                            overlay={<Popover id={"searchResultsDisplaySettings"}>
                                <Form onSubmit={handleMaxLinesPerResultSubmission}>
                                    <InputGroup>
                                        <InputGroup.Text>Max lines per result</InputGroup.Text>
                                        <Form.Control
                                            defaultValue={maxLinesPerResult}
                                            id={"maxLinesPerResult"}
                                            min={1}
                                            name={"maxLinesPerResult"}
                                            type={"number"}/>
                                    </InputGroup>
                                </Form>
                            </Popover>}
                        >
                            <Button
                                title={"Display Settings"}
                                type={"button"}
                                variant={"light"}
                            >
                                <FontAwesomeIcon icon={faCog}/>
                            </Button>
                        </OverlayTrigger>
                    </Col>
                </Row>
            </Container>
        </>
    );
};

export default SearchResultsHeader;
