import React from "react";

import {faCog} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
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

import "./SearchResultsHeader.scss";


/**
 * Renders the header for the search results, which includes the job ID, the number of results
 * found, and a control for setting the maximum number of lines per search result.
 *
 * @param {number} jobId of the search job
 * @param {number} numResultsOnServer of the search job
 * @param {number} maxLinesPerResult to display
 * @param {function} setMaxLinesPerResult callback to set setMaxLinesPerResult
 * @returns {JSX.Element}
 */
const SearchResultsHeader = ({
    jobId,
    numResultsOnServer,
    maxLinesPerResult,
    setMaxLinesPerResult,
}) => {
    const handleMaxLinesPerResultSubmission = (e) => {
        e.preventDefault();
        const value = parseInt(e.target.elements["maxLinesPerResult"].value);
        if (value > 0) {
            setMaxLinesPerResult(value);
        }
    };

    return (<>
        <Container fluid={true}>
            <Row className={"search-results-title-bar"}>
                <Col>
                    <span className={"search-results-count"}>
                        Job ID {jobId} | Results count: {numResultsOnServer}
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
                                        id={"maxLinesPerResult"}
                                        name={"maxLinesPerResult"}
                                        type={"number"}
                                        defaultValue={maxLinesPerResult}
                                        min={1}
                                    />
                                </InputGroup>
                            </Form>
                        </Popover>}>
                        {(0 < numResultsOnServer) ? <Button type={"button"}
                                                            variant={"light"}
                                                            title={"Display Settings"}>
                            <FontAwesomeIcon icon={faCog}/>
                        </Button> : <></>}
                    </OverlayTrigger>
                </Col>
            </Row>
        </Container>
    </>);
};

export default SearchResultsHeader;
